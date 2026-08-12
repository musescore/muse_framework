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

#include "diagnosticscommandsregister.h"

#include "../diagnosticscommands.h"

using namespace muse;
using namespace muse::rcommand;
using namespace muse::diagnostics;

static const std::vector<CommandInfo> s_commandInfos = {
    CommandInfo{
        DIAGNOSTICS_SAVE_FILES_COMMAND,
        TranslatableString("diagnostics", "Save diagnostic files"),
        TranslatableString("diagnostics", "Save diagnostic files"),
        InputSchema(),
        Decoration()
    },
    CommandInfo{
        DIAGNOSTICS_SHOW_PATHS_COMMAND,
        TranslatableString("diagnostics", "Show paths…"),
        TranslatableString("diagnostics", "Show paths"),
        InputSchema(),
        Decoration()
    },
    CommandInfo{
        DIAGNOSTICS_SHOW_PROFILER_COMMAND,
        TranslatableString("diagnostics", "Show profiler…"),
        TranslatableString("diagnostics", "Show profiler"),
        InputSchema(),
        Decoration()
    },
    CommandInfo{
        DIAGNOSTICS_SHOW_GRAPHICSINFO_COMMAND,
        TranslatableString("diagnostics", "Show graphics info…"),
        TranslatableString("diagnostics", "Show graphics info"),
        InputSchema(),
        Decoration()
    },
    CommandInfo{
        DIAGNOSTICS_SHOW_NAVIGATION_TREE_COMMAND,
        TranslatableString("diagnostics", "Show navigation tree…"),
        TranslatableString("diagnostics", "Show navigation tree"),
        InputSchema(),
        Decoration()
    },
    CommandInfo{
        DIAGNOSTICS_SHOW_ACCESSIBLE_TREE_COMMAND,
        TranslatableString("diagnostics", "Show accessibility tree…"),
        TranslatableString("diagnostics", "Show accessibility tree"),
        InputSchema(),
        Decoration()
    },
    CommandInfo{
        DIAGNOSTICS_DUMP_ACCESSIBLE_TREE_COMMAND,
        TranslatableString("diagnostics", "Dump accessibility tree to console"),
        TranslatableString("diagnostics", "Dump accessibility tree to console"),
        InputSchema(),
        Decoration()
    },
    CommandInfo{
        DIAGNOSTICS_SHOW_ENGRAVING_ELEMENTS_COMMAND,
        TranslatableString("diagnostics", "Show engraving elements"),
        TranslatableString("diagnostics", "Show engraving elements"),
        InputSchema(),
        Decoration()
    },
    CommandInfo{
        DIAGNOSTICS_SHOW_ENGRAVING_UNDOSTACK_COMMAND,
        TranslatableString("diagnostics", "Show engraving undo stack"),
        TranslatableString("diagnostics", "Show engraving undo stack"),
        InputSchema(),
        Decoration()
    },
    CommandInfo{
        DIAGNOSTICS_SHOW_ENGRAVING_STYLE_COMMAND,
        TranslatableString("diagnostics", "Show engraving style options list"),
        TranslatableString("diagnostics", "Show engraving style options list"),
        InputSchema(),
        Decoration()
    },
    CommandInfo{
        DIAGNOSTICS_SHOW_ACTIONS_COMMAND,
        TranslatableString("diagnostics", "Show actions list"),
        TranslatableString("diagnostics", "Show actions list"),
        InputSchema(),
        Decoration()
    },
    CommandInfo{
        DIAGNOSTICS_SHOW_RCOMMANDS_COMMAND,
        TranslatableString("diagnostics", "Show rcommands list"),
        TranslatableString("diagnostics", "Show rcommands list"),
        InputSchema(),
        Decoration()
    },
    CommandInfo{
        DIAGNOSTICS_ACTIONS_QUERY_COMMAND,
        TranslatableString("diagnostics", "Test query action"),
        TranslatableString("diagnostics", "Test query action"),
        InputSchema(),
        Decoration()
    },
    CommandInfo{
        DIAGNOSTICS_ACTIONS_QUERY_PARAMS1_COMMAND,
        TranslatableString("diagnostics", "Test query action with params 1"),
        TranslatableString("diagnostics", "Test query action with params 1"),
        InputSchema({
            { "param1", Arg(DataType::String, u"Test parameter 1 (optional)") }
        }),
        Decoration()
    },
    CommandInfo{
        DIAGNOSTICS_ACTIONS_QUERY_PARAMS2_COMMAND,
        TranslatableString("diagnostics", "Test query action with params 2"),
        TranslatableString("diagnostics", "Test query action with params 2"),
        InputSchema({
            { "param1", Arg(DataType::String, u"Test parameter 1 (optional)") }
        }),
        Decoration()
    },
};

std::string DiagnosticsCommandsRegister::moduleName() const
{
    return "diagnostics";
}

const std::vector<muse::rcommand::Command>& DiagnosticsCommandsRegister::commandList() const
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

const std::vector<CommandInfo>& DiagnosticsCommandsRegister::commandInfoList() const
{
    return s_commandInfos;
}
