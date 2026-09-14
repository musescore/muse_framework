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

#include <QtQmlIntegration/qqmlintegration.h>

#include <QString>

#include "sorter.h"

namespace muse::uicomponents {
class SorterValue : public Sorter
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString roleName READ roleName WRITE setRoleName NOTIFY dataChanged)

public:
    explicit SorterValue(QObject* parent = nullptr);

    bool lessThan(const QModelIndex& sourceLeft, const QModelIndex& sourceRight, const SortFilterProxyModel&) override;

    QString roleName() const;
    void setRoleName(QString roleName);

private:
    QString m_roleName;
};
}
