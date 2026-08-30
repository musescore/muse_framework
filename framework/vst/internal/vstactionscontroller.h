/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2025 MuseScore Limited and others
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

#include "actions/actionable.h"

#include "modularity/ioc.h"
#include "actions/iactionsdispatcher.h"
#include "rcommand/commandable.h"
#include "rcommand/icommanddispatcher.h"
#include "interactive/iinteractive.h"
#include "interactive/iinteractiveuriregister.h"
#include "../ivstinstancesregister.h"
#include "../ivstconfiguration.h"
#include "types/ret.h"

namespace muse::vst {
class VstActionsController : public actions::Actionable, public rcommand::Commandable, public muse::Contextable
{
    muse::GlobalInject<IVstConfiguration> configuration;
    muse::GlobalInject<interactive::IInteractiveUriRegister> interactiveUriRegister;
    muse::GlobalInject<IVstInstancesRegister> instancesRegister;
    muse::ContextInject<actions::IActionsDispatcher> dispatcher = { this };
    muse::ContextInject<rcommand::ICommandDispatcher> commandDispatcher = { this };
    muse::ContextInject<IInteractive> interactive = { this };

public:
    VstActionsController(const muse::modularity::ContextPtr& iocCtx)
        : muse::Contextable(iocCtx)
    {
    }

    void init();

    muse::Ret fxEditor(const rcommand::Params& params);
    muse::Ret instEditor(const rcommand::Params& params);

    void editorOperation(const std::string& operation, int instanceId, bool sync);

    void setupUsedView();
    void useView(bool isNew);
    bool isUsedNewView() const;
    bool actionChecked(const actions::ActionCode& act) const;
    async::Channel<actions::ActionCodeList> actionCheckedChanged() const;

private:

    async::Channel<actions::ActionCodeList> m_actionCheckedChanged;
};
}
