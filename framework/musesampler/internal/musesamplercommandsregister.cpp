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

 #include "musesamplercommandsregister.h"

 #include "../musesamplercommands.h"

using namespace muse;
using namespace muse::rcommand;
using namespace muse::musesampler;

static const std::vector<CommandInfo> s_commandInfos = {
    CommandInfo{
        MUSESAMPLER_CHECK_COMMAND,
        TranslatableString("musesampler", "Check"),
        TranslatableString("musesampler", "Check MuseSampler"),
        InputSchema(),
        Decoration()
    },
    CommandInfo{
        MUSESAMPLER_RELOAD_COMMAND,
        TranslatableString("musesampler", "Reload"),
        TranslatableString("musesampler", "Reload MuseSampler"),
        InputSchema(),
        Decoration()
    },
};

std::string MuseSamplerCommandsRegister::moduleName() const
{
    return "musesampler";
}

const std::vector<muse::rcommand::Command>& MuseSamplerCommandsRegister::commandList() const
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

const std::vector<CommandInfo>& MuseSamplerCommandsRegister::commandInfoList() const
{
    return s_commandInfos;
}
