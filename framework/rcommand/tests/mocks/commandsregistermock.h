/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <gmock/gmock.h>

#include "rcommand/icommandsregister.h"

namespace muse::rcommand {
class CommandsRegisterMock : public ICommandsRegister
{
public:
    MOCK_METHOD(void, reg, (const IModuleCommandsRegisterPtr& module), (override));
    MOCK_METHOD(void, unreg, (const IModuleCommandsRegisterPtr& module), (override));
    MOCK_METHOD(IModuleCommandsRegisterPtr, moduleRegister, (const std::string& moduleName), (const, override));

    MOCK_METHOD(std::vector<CommandInfo>, commandInfoList, (), (const, override));
    MOCK_METHOD(const CommandInfo&, commandInfo, (const Command& command), (const, override));
    MOCK_METHOD(async::Notification, commandInfoListChanged, (), (const, override));

    MOCK_METHOD(const std::string&, commandModuleName, (const Command& command), (const, override));
};
}
