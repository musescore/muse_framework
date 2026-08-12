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

#include "multiwindowscommandscontroller.h"

#include "rcommand/actiontocommand.h"

#include "../multiwindowscommands.h"

#include "types/uri.h"

using namespace muse;
using namespace muse::mi;
using namespace muse::rcommand;

static const muse::UriQuery DEV_SHOW_INFO_URI("muse://devtools/multiwindows/info?modal=false");

void MultiWindowsCommandsController::init()
{
    commandDispatcher()->onRequest(this, MULTIWINDOWS_DEV_SHOW_INFO_COMMAND, [this]() {
        showInfo();
        return muse::make_ok();
    });

    // compat
    {
        static const std::vector<ActionToCommand> actionToCommands = {
            { "multiwindows-dev-show-info", MULTIWINDOWS_DEV_SHOW_INFO_COMMAND, {} },
        };

        rcommand::registerActionToCommand(this, actionToCommands, commandDispatcher(), dispatcher());
    }
}

void MultiWindowsCommandsController::showInfo()
{
    if (!interactive()->isOpened(DEV_SHOW_INFO_URI.uri()).val) {
        interactive()->open(DEV_SHOW_INFO_URI);
    }
}
