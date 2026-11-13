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

#ifndef MUSE_DOCK_DOCKWINDOWACTIONSCONTROLLER_H
#define MUSE_DOCK_DOCKWINDOWACTIONSCONTROLLER_H

#include "rcommand/commandable.h"
#include "global/types/ret.h"

#include "modularity/ioc.h"
#include "rcommand/icommanddispatcher.h"
#include "../idockwindowprovider.h"

#include "muse_framework_config.h"
#ifdef MUSE_MODULE_ACTIONS_SUPPORT
#include "actions/actionable.h"
#include "actions/iactionsdispatcher.h"
#endif

namespace muse::dock {
class DockWindowActionsController : public muse::Contextable, public muse::rcommand::Commandable
#ifdef MUSE_MODULE_ACTIONS_SUPPORT
    , public muse::actions::Actionable
#endif
{
    muse::ContextInject<IDockWindowProvider> dockWindowProvider = { this };
    muse::ContextInject<rcommand::ICommandDispatcher> commandsDispatcher = { this };

#ifdef MUSE_MODULE_ACTIONS_SUPPORT
    muse::ContextInject<actions::IActionsDispatcher> dispatcher = { this };
#endif

public:
    DockWindowActionsController(const muse::modularity::ContextPtr& iocCtx)
        : muse::Contextable(iocCtx) {}

    void init();

private:
    muse::Ret setDockOpen(const rcommand::Params& params);
    muse::Ret toggleOpened(const rcommand::Params& params);
    muse::Ret toggleFloating(const rcommand::Params& params);

    muse::Ret restoreDefaultLayout();

    IDockWindow* window() const;
};
}

#endif // MUSE_DOCK_DOCKWINDOWACTIONSCONTROLLER_H
