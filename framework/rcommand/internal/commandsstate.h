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

#include "../icommandsstate.h"
#include "global/async/asyncable.h"
#include "global/modularity/ioc.h"
#include "../icommandsregister.h"

namespace muse::rcommand {
class CommandsState : public ICommandsState, public async::Asyncable
{
    GlobalInject<ICommandsRegister> commandsRegister;

public:
    CommandsState() = default;

    void reg(const IModuleCommandsStatePtr& module) override;
    void unreg(const IModuleCommandsStatePtr& module) override;

    CommandState commandState(const Command& command) const override;
    async::Channel<Command, CommandState> commandStateChanged() const override;

private:

    std::map<std::string, IModuleCommandsStatePtr> m_modules;
    async::Channel<Command, CommandState> m_commandStateChanged;

    mutable std::map<Command, CommandState> m_cache;
};
}
