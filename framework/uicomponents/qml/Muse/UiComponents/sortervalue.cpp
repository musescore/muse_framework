/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "sortervalue.h"

#include "global/log.h"

#include "sortfilterproxymodel.h"

using namespace muse::uicomponents;

SorterValue::SorterValue(QObject* parent)
    : Sorter(parent)
{
}

bool SorterValue::lessThan(const QModelIndex& sourceLeft, const QModelIndex& sourceRight,
                           const SortFilterProxyModel& proxyModel)
{
    const int roleId = proxyModel.roleIdFromName(m_roleName);
    if (roleId < 0) {
        return sourceLeft < sourceRight;
    }

    const QAbstractItemModel* sourceModel = proxyModel.sourceModel();
    IF_ASSERT_FAILED(sourceModel) {
        return sourceLeft < sourceRight;
    }

    const QVariant leftData = sourceModel->data(sourceLeft, roleId);
    const QVariant rightData = sourceModel->data(sourceRight, roleId);
    const QPartialOrdering ordering = QVariant::compare(leftData, rightData);
    if (ordering == QPartialOrdering::Unordered || ordering == QPartialOrdering::Equivalent) {
        return sourceLeft < sourceRight;
    }

    return ordering == QPartialOrdering::Less;
}

QString SorterValue::roleName() const
{
    return m_roleName;
}

void SorterValue::setRoleName(QString roleName)
{
    if (m_roleName == roleName) {
        return;
    }

    m_roleName = roleName;
    emit dataChanged();
}
