/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore/Audacity CLA applies
 *
 * Copyright (C) MuseScore/Audacity and others
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

#include <string>
#include <vector>
#include <memory>

#include "commandtypes.h"

namespace muse::rcommand {
class IModuleCommandsRegister
{
public:

    virtual ~IModuleCommandsRegister() = default;

    virtual std::string moduleName() const = 0;

    virtual const std::vector<Command>& commandList() const = 0;
    virtual const std::vector<CommandInfo>& commandInfoList() const = 0;
};

using IModuleCommandsRegisterPtr = std::shared_ptr<IModuleCommandsRegister>;
}
