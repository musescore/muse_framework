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

#pragma once

#include "rcommand/imodulecommandsregister.h"

#include "global/async/asyncable.h"

#include "global/modularity/ioc.h"
#include "../iextensionsregister.h"

namespace muse::extensions {
class ExtensionsCommandsRegister : public rcommand::IModuleCommandsRegister, public async::Asyncable
{
    GlobalInject<IExtensionsRegister> extensionsRegister;

public:
    ExtensionsCommandsRegister() = default;

    void init();

    std::string moduleName() const override;

    const std::vector<muse::rcommand::Command>& commandList() const override;
    const std::vector<muse::rcommand::CommandInfo>& commandInfoList() const override;
    async::Notification commandListChanged() const override;

private:

    void reload();

    std::vector<rcommand::CommandInfo> m_commandInfos;
    mutable std::vector<rcommand::Command> m_commands;
    async::Notification m_commandListChanged;
};
}
