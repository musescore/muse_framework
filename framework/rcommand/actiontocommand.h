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

#include <memory>

#include "commandtypes.h"
#include "icommanddispatcher.h"
#include "actions/actiontypes.h"
#include "actions/iactionsdispatcher.h"

//! NOTE compat action to command convertor
/**
 * Example:
    {
        static const std::vector<ActionToCommand> actionToCommand = {
            { "file-new", NEW_PROJECT_COMMAND, {} },                                                    // no args - no convertor
            { "file-save-at", PROJECT_SAVE_AT_COMMAND, make_conv({ { "path", param<io::path_t> } }) },  // general converotor
            { "file-open", OPEN_PROJECT_COMMAND, openArgs },                                            // custom convertor

        };

        rcommand::registerActionToCommand(this, actionToCommand, commandDispatcher(), dispatcher());
    }
 */

namespace muse::rcommand {
template<typename T>
constexpr auto param = [](const actions::ActionData& args, int index) -> Val {
    return Val(args.arg<T>(index));
};

struct ParamSpec {
    std::string name;
    std::function<Val(const actions::ActionData&, int)> param;
};

using Convertor = std::function<muse::rcommand::CommandQuery (const rcommand::Command&, const actions::ActionData&)>;
struct ActionToCommand {
    actions::ActionCode actionCode;
    muse::rcommand::Command command;
    Convertor convertor;
};

static inline Convertor make_conv(const std::vector<ParamSpec>& specs)
{
    return [specs](const rcommand::Command& command, const actions::ActionData& args) -> muse::rcommand::CommandQuery {
        muse::rcommand::CommandQuery query(command);
        if (args.empty()) {
            return query;
        }

        IF_ASSERT_FAILED(args.count() <= static_cast<int>(specs.size())) {
            return query;
        }

        for (int i = 0; i < args.count(); ++i) {
            query.set(specs[i].name, specs[i].param(args, i));
        }
        return query;
    };
}

static inline void registerActionToCommand(actions::Actionable* actionable,
                                           const std::vector<ActionToCommand>& actionToCommand,
                                           std::shared_ptr<ICommandDispatcher> cd,
                                           std::shared_ptr<actions::IActionsDispatcher> ad)
{
    for (const auto& atc : actionToCommand) {
        ad->reg(actionable, atc.actionCode, [cd, atc](const actions::ActionData& args) {
            if (atc.convertor) {
                return cd->dispatch(atc.convertor(atc.command, args));
            }
            return cd->dispatch(atc.command);
        });
    }
}
}
