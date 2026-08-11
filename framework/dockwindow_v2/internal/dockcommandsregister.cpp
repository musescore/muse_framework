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

#include "dockcommandsregister.h"

#include "../dockcommands.h"
#include "rcommand/commandtypes.h"

using namespace muse;
using namespace muse::rcommand;
using namespace muse::dock;

static const std::vector<CommandInfo> s_commandInfos = {
    CommandInfo {
        DOCK_SET_OPEN_COMMAND,
        TranslatableString("dock", "Set Open"),
        TranslatableString("dock", "Dock: Set Open"),
        InputSchema({
            { "dock_name", Arg(DataType::String, u"Dock name") },
            { "open", Arg(DataType::Boolean, u"Open") }
        }),
        Decoration()
    },
    CommandInfo {
        DOCK_TOGGLE_COMMAND,
        TranslatableString("dock", "Toggle dock"),
        TranslatableString("dock", "Dock: Toggle dock"),
        InputSchema({ { "dock_name", Arg(DataType::String, u"Dock name") } }),
        Decoration()
    },
    CommandInfo {
        DOCK_TOGGLE_FLOATING_COMMAND,
        TranslatableString("dock", "Toggle dock floating"),
        TranslatableString("dock", "Dock: Toggle dock floating"),
        InputSchema({ { "dock_name", Arg(DataType::String, u"Dock name") } }),
        Decoration()
    },
    CommandInfo {
        DOCK_RESTORE_DEFAULT_LAYOUT_COMMAND,
        TranslatableString("dock", "Restore default layout"),
        TranslatableString("dock", "Dock: Restore default layout"),
        InputSchema(),
        Decoration()
    }
};

std::string DockCommandsRegister::moduleName() const
{
    return "dock";
}

const std::vector<muse::rcommand::Command>& DockCommandsRegister::commandList() const
{
    static std::vector<muse::rcommand::Command> commands;
    if (commands.empty()) {
        commands.reserve(s_commandInfos.size());
        for (const auto& info : s_commandInfos) {
            commands.push_back(info.command);
        }
    }
    return commands;
}

const std::vector<CommandInfo>& DockCommandsRegister::commandInfoList() const
{
    return s_commandInfos;
}
