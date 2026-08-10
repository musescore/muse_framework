/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited and others
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

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QList>
#include <QString>
#include <QVariant>

namespace muse::uicomponents {
class SectionedModel : public QAbstractListModel
{
public:
    enum Roles {
        RoleTitle = Qt::UserRole + 1,
        RoleGroup
    };

    struct Item {
        QString title;
        QString group;
    };

    //! NOTE: The default items are not sorted by the group
    static QList<Item> defaultItems()
    {
        using namespace Qt::StringLiterals;

        return {
            { u"b1"_s, u"B"_s },
            { u"a2"_s, u"A"_s },
            { u"x"_s, QString() },
            { u"a1"_s, u"A"_s },
            { u"b2"_s, u"B"_s }
        };
    }

    explicit SectionedModel(const QList<Item>& items = defaultItems())
        : m_items(items) {}

    int rowCount(const QModelIndex& parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : m_items.size();
    }

    QVariant data(const QModelIndex& index, int role) const override
    {
        if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
            return QVariant();
        }

        const Item& item = m_items.at(index.row());

        switch (role) {
        case RoleTitle: return item.title;
        case RoleGroup: return item.group;
        }

        return QVariant();
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {
            { RoleTitle, "title" },
            { RoleGroup, "group" }
        };
    }

    //! NOTE: Notifies without the roles, like most of the models do
    void setTitle(int row, const QString& title)
    {
        m_items[row].title = title;
        emit dataChanged(index(row), index(row));
    }

    void setGroup(int row, const QString& group)
    {
        m_items[row].group = group;
        emit dataChanged(index(row), index(row));
    }

private:
    QList<Item> m_items;
};
}
