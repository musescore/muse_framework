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

#include "commandsregister.h"

#include "log.h"

using namespace muse;
using namespace muse::rcommand;

void CommandsRegister::reg(const IModuleCommandsPtr& module)
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
}

void CommandsRegister::unreg(const IModuleCommandsPtr& module)
{
    IF_ASSERT_FAILED(module) {
        return;
    }

    const std::string& moduleName = module->moduleName();
    IF_ASSERT_FAILED(!moduleName.empty()) {
        return;
    }

    m_modules.erase(moduleName);
}

std::vector<CommandInfo> CommandsRegister::commandList() const
{
    std::vector<CommandInfo> commands;
    for (const auto& module : m_modules) {
        const auto& infos = module.second->commandInfos();
        commands.insert(commands.end(), infos.begin(), infos.end());
    }
    return commands;
}
