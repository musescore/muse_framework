/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) MuseScore Limited and others
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

#include "actions/actionable.h"

#include "modularity/ioc.h"
#include "actions/iactionsdispatcher.h"
#include "rcommand/commandable.h"
#include "rcommand/icommanddispatcher.h"
#include "interactive/iinteractive.h"

namespace muse::mi {
class MultiWindowsCommandsController : public Contextable, public actions::Actionable, public rcommand::Commandable
{
    ContextInject<actions::IActionsDispatcher> dispatcher = { this };
    ContextInject<rcommand::ICommandDispatcher> commandDispatcher = { this };
    ContextInject<IInteractive> interactive = { this };

public:
    MultiWindowsCommandsController(const modularity::ContextPtr& iocCtx)
        : Contextable(iocCtx) {}

    void init();

private:
    void showInfo();
};
}
