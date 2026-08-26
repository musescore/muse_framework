/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
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

#include "dockwindowactionscontroller.h"

#include "rcommand/actiontocommand.h"

#include "../idockwindow.h"
#include "../dockcommands.h"
#include "rcommand/commandtypes.h"

using namespace muse;
using namespace muse::dock;
using namespace muse::actions;
using namespace muse::rcommand;

static CommandQuery setOpenConv(const Command& command, const ActionData& args)
{
    CommandQuery query(command);
    if (args.count() < 2) {
        return query;
    }
    query.addParam("dock_name", Val(args.arg<QString>(0).toStdString()));
    query.addParam("open", Val(args.arg<bool>(1)));
    return query;
}

static CommandQuery toggleConv(const Command& command, const ActionData& args)
{
    CommandQuery query(command);
    if (args.count() < 1) {
        return query;
    }
    query.addParam("dock_name", Val(args.arg<QString>(0).toStdString()));
    return query;
}

void DockWindowActionsController::init()
{
    auto cd = commandsDispatcher();
    cd->onRequest(this, DOCK_SET_OPEN_COMMAND, [this](const rcommand::Params& params) { return setDockOpen(params); });
    cd->onRequest(this, DOCK_TOGGLE_COMMAND, [this](const rcommand::Params& params) { return toggleOpened(params); });
    cd->onRequest(this, DOCK_TOGGLE_FLOATING_COMMAND, [this](const rcommand::Params& params) { return toggleFloating(params); });
    cd->onRequest(this, DOCK_RESTORE_DEFAULT_LAYOUT_COMMAND, [this]() { return restoreDefaultLayout(); });

    // compat
    {
        static std::vector<ActionToCommand> actionsToCommands = {
            { "dock-set-open", DOCK_SET_OPEN_COMMAND, setOpenConv },
            { "dock-toggle", DOCK_TOGGLE_COMMAND, toggleConv },
            { "dock-toggle-floating", DOCK_TOGGLE_FLOATING_COMMAND, toggleConv },
            { "dock-restore-default-layout", DOCK_RESTORE_DEFAULT_LAYOUT_COMMAND, {} },
        };

        rcommand::registerActionToCommand(this, actionsToCommands, commandsDispatcher(), dispatcher());
    }
}

muse::Ret DockWindowActionsController::setDockOpen(const rcommand::Params& params)
{
    if (!(params.contains("dock_name") && params.contains("open"))) {
        return muse::make_ret(Ret::Code::BadArgs);
    }
    QString dockName = QString::fromStdString(params.at("dock_name").toString());
    bool open = params.at("open").toBool();
    window()->setDockOpen(dockName, open);
    return muse::make_ok();
}

muse::Ret DockWindowActionsController::toggleOpened(const rcommand::Params& params)
{
    if (!params.contains("dock_name")) {
        return muse::make_ret(Ret::Code::BadArgs);
    }

    QString dockName = QString::fromStdString(params.at("dock_name").toString());
    window()->toggleDock(dockName);
    return muse::make_ok();
}

muse::Ret DockWindowActionsController::toggleFloating(const rcommand::Params& params)
{
    if (!params.contains("dock_name")) {
        return muse::make_ret(Ret::Code::BadArgs);
    }

    QString dockName = QString::fromStdString(params.at("dock_name").toString());
    window()->toggleDockFloating(dockName);
    return muse::make_ok();
}

IDockWindow* DockWindowActionsController::window() const
{
    return dockWindowProvider()->window();
}

muse::Ret DockWindowActionsController::restoreDefaultLayout()
{
    window()->restoreDefaultLayout();
    return muse::make_ok();
}
