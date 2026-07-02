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

void CommandsRegister::reg(const IModuleCommandsRegisterPtr& module)
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

    for (const auto& info : module->commandInfoList()) {
        m_commandModuleNames[info.command] = moduleName;
    }
}

void CommandsRegister::unreg(const IModuleCommandsRegisterPtr& module)
{
    IF_ASSERT_FAILED(module) {
        return;
    }

    const std::string& moduleName = module->moduleName();
    IF_ASSERT_FAILED(!moduleName.empty()) {
        return;
    }

    m_modules.erase(moduleName);

    for (const auto& info : module->commandInfoList()) {
        m_commandModuleNames.erase(info.command);
    }
}

IModuleCommandsRegisterPtr CommandsRegister::moduleRegister(const std::string& moduleName) const
{
    auto it = m_modules.find(moduleName);
    if (it != m_modules.end()) {
        return it->second;
    }

    return nullptr;
}

std::vector<CommandInfo> CommandsRegister::commandInfoList() const
{
    std::vector<CommandInfo> commands;
    for (const auto& module : m_modules) {
        const auto& infos = module.second->commandInfoList();
        commands.insert(commands.end(), infos.begin(), infos.end());
    }
    return commands;
}

const CommandInfo& CommandsRegister::commandInfo(const Command& command) const
{
    for (const auto& module : m_modules) {
        const auto& infos = module.second->commandInfoList();
        for (const auto& info : infos) {
            if (info.command == command) {
                return info;
            }
        }
    }

    static CommandInfo null;
    return null;
}

const std::string& CommandsRegister::commandModuleName(const Command& command) const
{
    auto it = m_commandModuleNames.find(command);
    if (it != m_commandModuleNames.end()) {
        return it->second;
    }

    static const std::string empty;
    return empty;
}
