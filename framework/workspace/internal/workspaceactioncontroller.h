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
#ifndef MUSE_WORKSPACE_WORKSPACEACTIONCONTROLLER_H
#define MUSE_WORKSPACE_WORKSPACEACTIONCONTROLLER_H

#include "actions/actionable.h"

#include "modularity/ioc.h"
#include "interactive/iinteractive.h"
#include "actions/iactionsdispatcher.h"
#include "rcommand/commandable.h"
#include "rcommand/icommanddispatcher.h"
#include "iworkspaceconfiguration.h"
#include "iworkspacemanager.h"

namespace muse::workspace {
class WorkspaceActionController : public Contextable, public actions::Actionable, public rcommand::Commandable
{
    GlobalInject<IWorkspaceConfiguration> configuration;
    ContextInject<actions::IActionsDispatcher> dispatcher = { this };
    ContextInject<rcommand::ICommandDispatcher> commandDispatcher = { this };
    ContextInject<IInteractive> interactive = { this };
    ContextInject<IWorkspaceManager> manager = { this };

public:
    WorkspaceActionController(const modularity::ContextPtr& iocCtx)
        : Contextable(iocCtx) {}

    void init();

private:
    muse::Ret selectWorkspace(const muse::rcommand::CommandQuery& query);
    void openWorkspacesConfigure();
    void createNewWorkspace();
};
}

#endif // MUSE_WORKSPACE_WORKSPACEACTIONCONTROLLER_H
