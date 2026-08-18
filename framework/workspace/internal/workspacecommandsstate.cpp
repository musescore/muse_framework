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

#include "workspacecommandsstate.h"

#include "../workspacecommands.h"

using namespace muse;
using namespace muse::rcommand;
using namespace muse::workspace;

std::string WorkspaceCommandsState::moduleName() const
{
    return "workspace";
}

void WorkspaceCommandsState::init()
{
    m_moduleRegister = commandsRegister()->moduleRegister(moduleName());
    IF_ASSERT_FAILED(m_moduleRegister) {
        return;
    }

    updateCommandStates();
}

void WorkspaceCommandsState::deinit()
{
}

void WorkspaceCommandsState::updateCommandStates(const std::vector<Command>& commands)
{
    IF_ASSERT_FAILED(m_moduleRegister) {
        return;
    }

    const auto& commandList = commands.empty() ? m_moduleRegister->commandList() : commands;

    for (const auto& command : commandList) {
        CommandState newState = commandState(command);
        if (m_commandStates[command] != newState) {
            m_commandStates[command] = newState;
            m_commandStateChanged.send(command, newState);
        }
    }
}

CommandState WorkspaceCommandsState::commandState(const Command& command) const
{
    if (command == WORKSPACE_SELECT_COMMAND
        || command == WORKSPACES_CONFIGURE_COMMAND
        || command == WORKSPACE_CREATE_COMMAND) {
        return CommandState(true, false);
    }

    return CommandState(true, false);
}

async::Channel<Command, CommandState> WorkspaceCommandsState::commandStateChanged() const
{
    return m_commandStateChanged;
}
