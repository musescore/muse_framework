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

#include "extensionscommandsstate.h"

#include "../extensionscommands.h"

#include "log.h"

using namespace muse;
using namespace muse::rcommand;
using namespace muse::extensions;

std::string ExtensionsCommandsState::moduleName() const
{
    return "extensions";
}

void ExtensionsCommandsState::init()
{
    m_moduleRegister = commandsRegister()->moduleRegister(moduleName());
    IF_ASSERT_FAILED(m_moduleRegister) {
        return;
    }

    if (contextResolver()) {
        contextResolver()->contextChanged().onNotify(this, [this]() {
            updateCommandStates();
        });
    }

    updateCommandStates();
}

void ExtensionsCommandsState::deinit()
{
}

void ExtensionsCommandsState::updateCommandStates(const std::vector<Command>& commands)
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

CommandState ExtensionsCommandsState::commandState(const Command& command) const
{
    if (command == OPEN_APIDUMP_COMMAND) {
        return CommandState(true, true);
    }

    if (!contextResolver()) {
        return CommandState(true, false);
    }

    ExtensionUri extensionUri = uriFromCommand(command);
    const Manifest& manifest = extensionsRegister()->manifest(extensionUri);
    IF_ASSERT_FAILED(manifest.isValid()) {
        return CommandState(false, false);
    }

    if (!contextResolver()->isContextAllowed(manifest.context)) {
        return CommandState(false, false);
    }

    return CommandState(true, false);
}

async::Channel<Command, CommandState> ExtensionsCommandsState::commandStateChanged() const
{
    return m_commandStateChanged;
}
