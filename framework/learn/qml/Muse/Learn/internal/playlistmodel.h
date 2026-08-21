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

#pragma once

#include <QAbstractListModel>

#include "learn/learntypes.h"

namespace muse::learn {
class PlaylistModel final : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit PlaylistModel(QObject* parent = nullptr);

    const Playlist& playlist();
    void setPlaylist(const Playlist&);

    int rowCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int roleId) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    enum Roles {
        RoleTitle = Qt::UserRole,
        RoleAuthor,
        RoleDuration,
        RoleUrl,
        RoleThumbnailUrl,
        RoleSearchKey,
    };

    Playlist m_playlist;
};
}
