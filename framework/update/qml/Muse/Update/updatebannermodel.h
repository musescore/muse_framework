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

#include <QObject>
#include <qqmlintegration.h>

#include "async/asyncable.h"

#include "modularity/ioc.h"

#include "iappupdatescenario.h"

namespace muse::update {
class UpdateBannerModel : public QObject, public Contextable, public async::Asyncable
{
    Q_OBJECT

    Q_PROPERTY(bool updateReady READ updateReady NOTIFY updateReadyChanged)
    Q_PROPERTY(QString updateVersion READ updateVersion NOTIFY updateReadyChanged)

    QML_ELEMENT

    ContextInject<IAppUpdateScenario> scenario = { this };

public:
    explicit UpdateBannerModel(QObject* parent = nullptr);

    Q_INVOKABLE void load();
    Q_INVOKABLE void install();

    bool updateReady() const;
    QString updateVersion() const;

signals:
    void updateReadyChanged();
};
}
