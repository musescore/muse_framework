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
 #pragma once

 #include <map>
 #include <string>

 #include "../icommandsregister.h"

namespace muse::rcommand {
class CommandsRegister : public ICommandsRegister
{
public:
    CommandsRegister() = default;

    void reg(const IModuleCommandsPtr& module) override;
    void unreg(const IModuleCommandsPtr& module) override;
    std::vector<CommandInfo> commandList() const override;

private:
    std::map<std::string, IModuleCommandsPtr> m_modules;
};
}
