/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
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

#include "pagetabsmodel.h"

using namespace muse::uicomponents;

PageTabsModel::PageTabsModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

QVariant PageTabsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount()) {
        return QVariant();
    }

    const PageTab& tab = m_tabs.at(index.row());
    switch (role) {
    case TitleRole: return tab.title;
    case UriRole: return tab.uri;
    case IsTitleBoldRole: return tab.isTitleBold;
    case IsCheckedRole: return isChecked(tab);
    }

    return QVariant();
}

int PageTabsModel::rowCount(const QModelIndex&) const
{
    return static_cast<int>(m_tabs.size());
}

QHash<int, QByteArray> PageTabsModel::roleNames() const
{
    static const QHash<int, QByteArray> roles = {
        { TitleRole, "title" },
        { UriRole, "uri" },
        { IsTitleBoldRole, "isTitleBold" },
        { IsCheckedRole, "isChecked" },
    };

    return roles;
}

void PageTabsModel::load()
{
}

QString PageTabsModel::currentUri() const
{
    return m_currentUri;
}

void PageTabsModel::setCurrentUri(const QString& uri)
{
    if (m_currentUri == uri) {
        return;
    }

    m_currentUri = uri;
    emit currentUriChanged();

    if (!m_tabs.isEmpty()) {
        emit dataChanged(index(0), index(rowCount() - 1), { IsCheckedRole });
    }
}

void PageTabsModel::setTabs(const QList<PageTab>& tabs)
{
    beginResetModel();
    m_tabs = tabs;
    endResetModel();
}

void PageTabsModel::setTab(int index, const PageTab& tab)
{
    if (index < 0 || index >= rowCount()) {
        return;
    }

    m_tabs[index] = tab;

    QModelIndex modelIndex = this->index(index);
    emit dataChanged(modelIndex, modelIndex);
}

bool PageTabsModel::isChecked(const PageTab& tab) const
{
    return tab.uri == m_currentUri || tab.alsoCheckedOnUris.contains(m_currentUri);
}
