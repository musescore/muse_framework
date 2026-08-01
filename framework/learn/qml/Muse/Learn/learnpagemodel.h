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

#include <qqmlregistration.h>

#include <QObject>
#include <QVariant>

#include "global/async/asyncable.h"
#include "global/modularity/ioc.h"

#include "learn/ilearnconfiguration.h"
#include "learn/ilearnservice.h"

#include "internal/playlistmodel.h"

namespace muse::learn {
class LearnPageModel : public QObject, public async::Asyncable
{
    Q_OBJECT
    QML_ELEMENT;

    Q_PROPERTY(QAbstractItemModel * startedPlaylist READ startedPlaylist CONSTANT)
    Q_PROPERTY(QAbstractItemModel * advancedPlaylist READ advancedPlaylist CONSTANT)

    GlobalInject<ILearnConfiguration> learnConfiguration;
    GlobalInject<ILearnService> learnService;

public:
    explicit LearnPageModel(QObject* parent = nullptr);

    PlaylistModel* startedPlaylist() const;
    PlaylistModel* advancedPlaylist() const;

    Q_INVOKABLE void load();
    Q_INVOKABLE QVariantMap classesAuthor() const;
    Q_INVOKABLE bool classesEnabled();

private:
    PlaylistModel* m_gettingStartedModel = nullptr;
    PlaylistModel* m_advancedModel = nullptr;
};
}
