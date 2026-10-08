/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited and others
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
#include "navigationcommandsregister.h"

#include "../navigationcommands.h"

using namespace muse;
using namespace muse::rcommand;
using namespace muse::ui;

static const std::vector<CommandInfo> s_commandInfos = {
    CommandInfo{
        ESCAPE_COMMAND,
        TranslatableString("navigation", "Escape"),
        TranslatableString("navigation", "Navigate escape"),
        InputSchema(),
        Decoration(),
        Availability::All
    },
    CommandInfo{
        NEXT_SECTION_COMMAND,
        TranslatableString("navigation", "Next section"),
        TranslatableString("navigation", "Navigate next section"),
        InputSchema(),
        Decoration(),
        Availability::All
    },
    CommandInfo{
        PREV_SECTION_COMMAND,
        TranslatableString("navigation", "Previous section"),
        TranslatableString("navigation", "Navigate previous section"),
        InputSchema(),
        Decoration(),
        Availability::All
    },
    CommandInfo{
        NEXT_PANEL_COMMAND,
        TranslatableString("navigation", "Next panel"),
        TranslatableString("navigation", "Navigate next panel"),
        InputSchema(),
        Decoration(),
        Availability::All
    },
    CommandInfo{
        PREV_PANEL_COMMAND,
        TranslatableString("navigation", "Previous panel"),
        TranslatableString("navigation", "Navigate previous panel"),
        InputSchema(),
        Decoration(),
        Availability::All
    },
    CommandInfo{
        NEXT_TAB_COMMAND,
        TranslatableString("navigation", "Next tab"),
        TranslatableString("navigation", "Navigate next tab"),
        InputSchema(),
        Decoration(),
        Availability::All
    },
    CommandInfo{
        PREV_TAB_COMMAND,
        TranslatableString("navigation", "Previous tab"),
        TranslatableString("navigation", "Navigate previous tab"),
        InputSchema(),
        Decoration(),
        Availability::All
    },
    CommandInfo{
        RIGHT_COMMAND,
        TranslatableString("navigation", "Right"),
        TranslatableString("navigation", "Navigate right"),
        InputSchema(),
        Decoration(),
        Availability::All
    },
    CommandInfo{
        LEFT_COMMAND,
        TranslatableString("navigation", "Left"),
        TranslatableString("navigation", "Navigate left"),
        InputSchema(),
        Decoration(),
        Availability::All
    },
    CommandInfo{
        UP_COMMAND,
        TranslatableString("navigation", "Up"),
        TranslatableString("navigation", "Navigate up"),
        InputSchema(),
        Decoration(),
        Availability::All
    },
    CommandInfo{
        DOWN_COMMAND,
        TranslatableString("navigation", "Down"),
        TranslatableString("navigation", "Navigate down"),
        InputSchema(),
        Decoration(),
        Availability::All
    },
    CommandInfo{
        FIRST_CONTROL_COMMAND,
        TranslatableString("navigation", "First control"),
        TranslatableString("navigation", "Navigate first control"),
        InputSchema(),
        Decoration(),
        Availability::All
    },
    CommandInfo{
        LAST_CONTROL_COMMAND,
        TranslatableString("navigation", "Last control"),
        TranslatableString("navigation", "Navigate last control"),
        InputSchema(),
        Decoration(),
        Availability::All
    },
    CommandInfo{
        NEXTROW_CONTROL_COMMAND,
        TranslatableString("navigation", "Next row control"),
        TranslatableString("navigation", "Navigate next row control"),
        InputSchema(),
        Decoration(),
        Availability::All
    },
    CommandInfo{
        PREVROW_CONTROL_COMMAND,
        TranslatableString("navigation", "Previous row control"),
        TranslatableString("navigation", "Navigate previous row control"),
        InputSchema(),
        Decoration(),
        Availability::All
    },
    CommandInfo{
        TRIGGER_CONTROL_COMMAND,
        TranslatableString("navigation", "Trigger control"),
        TranslatableString("navigation", "Navigate trigger control"),
        InputSchema(),
        Decoration(),
        Availability::All
    }
};

std::string NavigationCommandsRegister::moduleName() const
{
    return "navigation";
}

const std::vector<muse::rcommand::Command>& NavigationCommandsRegister::commandList() const
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

const std::vector<CommandInfo>& NavigationCommandsRegister::commandInfoList() const
{
    return s_commandInfos;
}
