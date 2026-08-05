/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2025 MuseScore Limited and others
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
#include "audioactionscontroller.h"

#include "../audiocommands.h"
#include "types/ret.h"

using namespace muse;
using namespace muse::audio;

void AudioActionsController::init()
{
    dispatcher()->onRequest(this, AUDIO_DEV_USE_DRIVER_MODE_COMMAND, [this]() { setMode(workmode::DriverMode); return muse::make_ok(); });
    dispatcher()->onRequest(this, AUDIO_DEV_USE_HYBRID_MODE_COMMAND, [this]() { setMode(workmode::HybridMode); return muse::make_ok(); });
}

void AudioActionsController::setMode(workmode::Mode m)
{
    workmode::setMode(m);
    m_modeChanged.notify();

    auto promise = interactive()->question("Changing the audio mode",
                                           "Restart required, do you want to perform it?",
                                           { IInteractive::Button::Yes, IInteractive::Button::No });

    promise.onResolve(nullptr, [this](const IInteractive::Result& res) {
        if (res.isButton(IInteractive::Button::Yes)) {
            application()->restart();
        }
    });
}

workmode::Mode AudioActionsController::mode() const
{
    return workmode::desiredMode();
}

async::Notification AudioActionsController:: modeChanged() const
{
    return m_modeChanged;
}
