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
#pragma once

#include "actions/actiontypes.h"
#include "global/async/asyncable.h"

#include "global/serialization/json.h"

#include "modularity/ioc.h"
#include "ishortcutsregister.h"
#include "actions/iactionsdispatcher.h"
#include "rcommand/icommanddispatcher.h"
#include "rcommand/commandable.h"
#include "ishortcutsconfiguration.h"

namespace muse::shortcuts {
class MigrationHelper : public Contextable, public async::Asyncable, public rcommand::Commandable
{
    GlobalInject<IShortcutsConfiguration> configuration;
    ContextInject<IShortcutsRegister> shortcutsRegister = { this };
    ContextInject<rcommand::ICommandDispatcher> commandDispatcher = { this };
    ContextInject<actions::IActionsDispatcher> actionsDispatcher = { this };

public:
    MigrationHelper(const modularity::ContextPtr& iocCtx)
        : Contextable(iocCtx) {}

    void init();
    void actionsToCommands();

private:

    void dispatchNext();
    void onCommand(const rcommand::Command& command);
    void onFinished();

    struct ScInfo {
        rcommand::Command command;
        actions::ActionCode action;
        std::vector<std::string> sequences;
    };

    ShortcutList m_shortcuts;
    ShortcutList::iterator m_currentIt;
    JsonArray m_jsonArray;

    std::vector<ScInfo> m_resultInfos;
};
}
