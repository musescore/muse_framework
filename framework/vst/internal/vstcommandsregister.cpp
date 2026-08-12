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

#include "vstcommandsregister.h"

#include "../vstcommands.h"

using namespace muse;
using namespace muse::rcommand;
using namespace muse::vst;

static const std::vector<CommandInfo> s_commandInfos = {
    CommandInfo{
        VST_USE_OLDVIEW_COMMAND,
        TranslatableString("vst", "Use old view"),
        TranslatableString("vst", "Use old view"),
        InputSchema(),
        Decoration(Checkable::Yes)
    },
    CommandInfo{
        VST_USE_NEWVIEW_COMMAND,
        TranslatableString("vst", "Use new view"),
        TranslatableString("vst", "Use new view"),
        InputSchema(),
        Decoration(Checkable::Yes)
    },
    CommandInfo{
        VST_OPEN_FX_EDITOR_COMMAND,
        TranslatableString("vst", "Open FX editor"),
        TranslatableString("vst", "Open FX editor"),
        InputSchema(),
        Decoration()
    },
    CommandInfo{
        VST_OPEN_INSTRUMENT_EDITOR_COMMAND,
        TranslatableString("vst", "Open instrument editor"),
        TranslatableString("vst", "Open instrument editor"),
        InputSchema(),
        Decoration()
    },
};

std::string VstCommandsRegister::moduleName() const
{
    return "vst";
}

const std::vector<muse::rcommand::Command>& VstCommandsRegister::commandList() const
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

const std::vector<CommandInfo>& VstCommandsRegister::commandInfoList() const
{
    return s_commandInfos;
}
