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

#include "workspacecommandsregister.h"

#include "../workspacecommands.h"

using namespace muse;
using namespace muse::rcommand;
using namespace muse::workspace;

static const std::vector<CommandInfo> s_commandInfos = {
    CommandInfo{
        SELECT_WORKSPACE_COMMAND,
        TranslatableString("workspace", "Select workspace"),
        TranslatableString("workspace", "Select workspace"),
        InputSchema({
            { "name", Arg(DataType::String, u"Workspace name") }
        }),
        Decoration()
    },
    CommandInfo{
        CONFIGURE_WORKSPACES_COMMAND,
        TranslatableString("workspace", "Edit workspaces..."),
        TranslatableString("workspace", "Edit workspaces"),
        InputSchema(),
        Decoration(ui::IconCode::Code::EDIT)
    },
    CommandInfo{
        CREATE_WORKSPACE_COMMAND,
        TranslatableString("workspace", "Create new workspace"),
        TranslatableString("workspace", "Create new workspace"),
        InputSchema(),
        Decoration(ui::IconCode::Code::PLUS)
    },
};

std::string WorkspaceCommandsRegister::moduleName() const
{
    return "workspace";
}

const std::vector<muse::rcommand::Command>& WorkspaceCommandsRegister::commandList() const
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

const std::vector<CommandInfo>& WorkspaceCommandsRegister::commandInfoList() const
{
    return s_commandInfos;
}
