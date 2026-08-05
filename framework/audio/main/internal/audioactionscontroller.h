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
#pragma once

#include "global/async/notification.h"

#include "modularity/ioc.h"
#include "rcommand/commandable.h"
#include "rcommand/icommanddispatcher.h"
#include "global/iapplication.h"
#include "interactive/iinteractive.h"

#include "audio/common/workmode.h"

namespace muse::audio {
class AudioActionsController : public muse::Contextable, public muse::rcommand::Commandable
{
    GlobalInject<IApplication> application;
    ContextInject<muse::rcommand::ICommandDispatcher> dispatcher = { this };
    ContextInject<IInteractive> interactive = { this };

public:
    AudioActionsController(const muse::modularity::ContextPtr& iocCtx)
        : Contextable(iocCtx) {}

    void init();

    workmode::Mode mode() const;
    async::Notification modeChanged() const;

private:

    void setMode(workmode::Mode m);

    async::Notification m_modeChanged;
};
}
