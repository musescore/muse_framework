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
#include "updatebannermodel.h"

using namespace muse::update;

UpdateBannerModel::UpdateBannerModel(QObject* parent)
    : QObject(parent), Contextable(muse::iocCtxForQmlObject(this))
{
}

void UpdateBannerModel::load()
{
    scenario()->hasReadyUpdateChanged().onNotify(this, [this]() {
        emit updateReadyChanged();
    });
}

bool UpdateBannerModel::updateReady() const
{
    return scenario()->hasReadyUpdate();
}

QString UpdateBannerModel::updateVersion() const
{
    return QString::fromStdString(scenario()->readyUpdateVersion());
}

void UpdateBannerModel::install()
{
    scenario()->installReadyUpdate();
}
