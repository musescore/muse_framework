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

#include "playlistmodel.h"

#include <QString>
#include <QStringList>

#include "global/log.h"

namespace muse::learn {
PlaylistModel::PlaylistModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

const Playlist& PlaylistModel::playlist()
{
    return m_playlist;
}

void PlaylistModel::setPlaylist(const Playlist& playlist)
{
    beginResetModel();
    m_playlist = playlist;
    endResetModel();
}

int PlaylistModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_playlist.size();
}

QVariant PlaylistModel::data(const QModelIndex& index, int roleId) const
{
    IF_ASSERT_FAILED(index.isValid() && index.row() < m_playlist.size()) {
        return QVariant();
    }

    const PlaylistItem& item = m_playlist[index.row()];

    switch (roleId) {
    case RoleTitle:
        return item.title;
    case RoleAuthor:
        return item.author;
    case RoleDuration: {
        // h:mm:ss for anything over an hour
        //    m:ss for anything under an hour
        //    0:ss for anything under a minute
        int seconds = item.durationSecs;
        int minutes = seconds / 60;
        seconds -= minutes * 60;
        int hours = minutes / 60;
        minutes -= hours * 60;

        return ((hours > 0)
                ? (QString::number(hours) + ":" + QString::number(minutes).rightJustified(2, '0'))
                : QString::number(minutes)
                ) + ":"
               + QString::number(seconds).rightJustified(2, '0');
    }
    case RoleUrl:
        return item.url;
    case RoleThumbnailUrl:
        return item.thumbnailUrl;
    case RoleSearchKey: {
        QStringList searchKeyItems;
        searchKeyItems << item.title
                       << item.author;

        return searchKeyItems.join(u' ');
    }

    default:
        UNREACHABLE;
        break;
    }

    return QVariant();
}

QHash<int, QByteArray> PlaylistModel::roleNames() const
{
    return QHash<int, QByteArray> {
        { RoleTitle, "title" },
        { RoleAuthor, "author" },
        { RoleDuration, "duration" },
        { RoleUrl, "url" },
        { RoleThumbnailUrl, "thumbnailUrl" },
        { RoleSearchKey, "searchKey" },
    };
}
}
