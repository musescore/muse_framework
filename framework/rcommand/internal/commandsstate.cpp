/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore/Audacity CLA applies
 *
 * Copyright (C) 2026 MuseScore/Audacity and others
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

#include "commandsstate.h"

using namespace muse;
using namespace muse::rcommand;

void CommandsState::reg(const IModuleCommandsStatePtr& module)
{
    IF_ASSERT_FAILED(module) {
        return;
    }

    const std::string& moduleName = module->moduleName();
    IF_ASSERT_FAILED(!moduleName.empty()) {
        return;
    }

    IF_ASSERT_FAILED(m_modules.find(moduleName) == m_modules.end()) {
        LOGW() << "module already registered: " << moduleName;
        return;
    }

    m_modules[moduleName] = module;

    module->commandStateChanged().onReceive(this, [this](const Command& command, const CommandState& state) {
        m_cache[command] = state;
        m_commandStateChanged.send(command, state);
    });

    module->init();
}

void CommandsState::unreg(const IModuleCommandsStatePtr& module)
{
    IF_ASSERT_FAILED(module) {
        return;
    }

    const std::string& moduleName = module->moduleName();
    IF_ASSERT_FAILED(!moduleName.empty()) {
        return;
    }

    module->commandStateChanged().disconnect(this);
    module->deinit();

    m_modules.erase(moduleName);
}

async::Channel<Command, CommandState> CommandsState::commandStateChanged() const
{
    return m_commandStateChanged;
}

CommandState CommandsState::commandState(const Command& command) const
{
    {
        auto it = m_cache.find(command);
        if (it != m_cache.end()) {
            return it->second;
        }
    }

    const std::string& moduleName = commandsRegister()->commandModuleName(command);
    IF_ASSERT_FAILED(!moduleName.empty()) {
        return CommandState();
    }

    auto mit = m_modules.find(moduleName);
    IF_ASSERT_FAILED(mit != m_modules.end()) {
        return CommandState();
    }

    const IModuleCommandsStatePtr& module = mit->second;
    IF_ASSERT_FAILED(module) {
        return CommandState();
    }

    CommandState state = module->commandState(command);

    m_cache[command] = state;

    return state;
}
