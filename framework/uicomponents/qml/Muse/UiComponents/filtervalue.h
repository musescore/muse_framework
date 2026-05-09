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

#pragma once

#include <qqmlintegration.h>

#include <QString>
#include <QVariant>

#include "filter.h"

namespace muse::uicomponents {
namespace CompareType {
Q_NAMESPACE;
QML_ELEMENT;

enum Type
{
    Equal,
    NotEqual,
    Contains
};
Q_ENUM_NS(Type)
}

class FilterValue : public Filter
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString roleName READ roleName WRITE setRoleName NOTIFY dataChanged)
    Q_PROPERTY(QVariant roleValue READ roleValue WRITE setRoleValue NOTIFY dataChanged)
    Q_PROPERTY(muse::uicomponents::CompareType::Type compareType READ compareType WRITE setCompareType NOTIFY dataChanged)

public:
    explicit FilterValue(QObject* parent = nullptr);

    bool acceptsRow(int sourceRow, const QModelIndex& sourceParent, const SortFilterProxyModel&) override;

    QString roleName() const;
    void setRoleName(QString roleName);

    QVariant roleValue() const;
    void setRoleValue(QVariant roleValue);

    CompareType::Type compareType() const;
    void setCompareType(muse::uicomponents::CompareType::Type compareType);

private:
    QString m_roleName;
    QVariant m_roleValue;
    CompareType::Type m_compareType = CompareType::Equal;
};
}
