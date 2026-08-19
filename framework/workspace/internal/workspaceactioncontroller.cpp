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
#include "workspaceactioncontroller.h"

#include "rcommand/actiontocommand.h"

#include "../workspacecommands.h"

#include "log.h"

using namespace muse::workspace;
using namespace muse::actions;
using namespace muse::rcommand;

void WorkspaceActionController::init()
{
    commandDispatcher()->onRequest(this, WORKSPACE_SELECT_COMMAND, [this](const CommandQuery& query) { return selectWorkspace(query); });
    commandDispatcher()->onRequest(this, WORKSPACES_CONFIGURE_COMMAND, [this]() { openWorkspacesConfigure(); return muse::make_ok(); });
    commandDispatcher()->onRequest(this, WORKSPACE_CREATE_COMMAND, [this]() { createNewWorkspace(); return muse::make_ok(); });

    // compat
    {
        static const std::vector<ActionToCommand> actionToCommands = {
            { "select-workspace", WORKSPACE_SELECT_COMMAND, make_conv({ { "name", param<std::string> } }) },
            { "configure-workspaces", WORKSPACES_CONFIGURE_COMMAND, {} },
            { "create-workspace", WORKSPACE_CREATE_COMMAND, {} }
        };

        rcommand::registerActionToCommand(this, actionToCommands, commandDispatcher(), dispatcher());
    }
}

muse::Ret WorkspaceActionController::selectWorkspace(const muse::rcommand::CommandQuery& query)
{
    std::string selectedWorkspace = query.param("name").toString();
    manager()->changeCurrentWorkspace(selectedWorkspace);
    return muse::make_ok();
}

void WorkspaceActionController::openWorkspacesConfigure()
{
    manager()->openConfigureWorkspacesDialog();
}

void muse::workspace::WorkspaceActionController::createNewWorkspace()
{
    manager()->createAndAppendNewWorkspace();
}
