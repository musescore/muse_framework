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

 #include "audiocommandsregister.h"

 #include "../audiocommands.h"
#include "rcommand/commandtypes.h"

using namespace muse;
using namespace muse::rcommand;
using namespace muse::audio;

static const std::vector<CommandInfo> s_commandInfos = {
    CommandInfo{
        AUDIO_DEV_USE_DRIVER_MODE_COMMAND,
        TranslatableString::untranslatable("Use Driver Mode"),
        TranslatableString::untranslatable("Use Driver Mode (developer mode)"),
        InputSchema(),
        Decoration(rcommand::Checkable::Yes)
    },
    CommandInfo{
        AUDIO_DEV_USE_HYBRID_MODE_COMMAND,
        TranslatableString::untranslatable("Use Hybrid Mode"),
        TranslatableString::untranslatable("Use Hybrid Mode (developer mode)"),
        InputSchema(),
        Decoration(rcommand::Checkable::Yes)
    },
};

std::string AudioCommandsRegister::moduleName() const
{
    return "audio";
}

const std::vector<muse::rcommand::Command>& AudioCommandsRegister::commandList() const
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

const std::vector<CommandInfo>& AudioCommandsRegister::commandInfoList() const
{
    return s_commandInfos;
}
