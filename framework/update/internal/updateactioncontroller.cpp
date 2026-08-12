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
#include "updateactioncontroller.h"

#include "rcommand/actiontocommand.h"

#include "../updatecommands.h"

using namespace muse::update;

void UpdateActionController::init()
{
    commandDispatcher()->onRequest(this, UPDATE_CHECK_COMMAND, [this]() {
        checkForAppUpdate();
        return muse::make_ok();
    });

    // compat
    {
        using namespace muse::rcommand;
        static const std::vector<ActionToCommand> actionToCommands = {
            { "check-update", UPDATE_CHECK_COMMAND, {} },
        };

        rcommand::registerActionToCommand(this, actionToCommands, commandDispatcher(), dispatcher());
    }
}

void UpdateActionController::checkForAppUpdate()
{
    appUpdateScenario()->checkForUpdate(/*manual*/ true);
}
