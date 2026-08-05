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

#include "audiocommandsstate.h"

#include "audioactionscontroller.h"

#include "../audiocommands.h"
#include "log.h"

using namespace muse;
using namespace muse::rcommand;
using namespace muse::audio;

std::string AudioCommandsState::moduleName() const
{
    return "audio";
}

void AudioCommandsState::init()
{
    m_moduleRegister = commandsRegister()->moduleRegister(moduleName());
    IF_ASSERT_FAILED(m_moduleRegister) {
        return;
    }

    m_controller->modeChanged().onNotify(this, [this]() {
        updateCommandStates();
    });

    updateCommandStates();
}

void AudioCommandsState::deinit()
{
    m_controller->modeChanged().disconnect(this);
}

void AudioCommandsState::updateCommandStates(const std::vector<Command>& commands)
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

CommandState AudioCommandsState::commandState(const Command& command) const
{
    if (command == AUDIO_DEV_USE_DRIVER_MODE_COMMAND) {
        return CommandState(true, m_controller->mode() == workmode::DriverMode);
    } else if (command == AUDIO_DEV_USE_HYBRID_MODE_COMMAND) {
        return CommandState(true, m_controller->mode() == workmode::HybridMode);
    }
    return CommandState(true, false);
}

async::Channel<Command, CommandState> AudioCommandsState::commandStateChanged() const
{
    return m_commandStateChanged;
}
