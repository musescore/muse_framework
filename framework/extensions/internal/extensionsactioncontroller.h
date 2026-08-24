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

#pragma once

#include "async/asyncable.h"
#include "rcommand/commandable.h"

#include "modularity/ioc.h"
#include "rcommand/icommandsregister.h"
#include "rcommand/icommanddispatcher.h"
#include "extensions/iextensionsprovider.h"
#include "interactive/iinteractive.h"
#include "extensionscommandsregister.h"

namespace muse::extensions {
class ExtensionsUiActions;
class ExtensionsActionController : public Contextable, public rcommand::Commandable, public async::Asyncable
{
    GlobalInject<muse::rcommand::ICommandsRegister> commandsRegister;
    ContextInject<extensions::IExtensionsProvider> provider = { this };
    ContextInject<IInteractive> interactive = { this };
    ContextInject<muse::rcommand::ICommandDispatcher> commandDispatcher = { this };

public:
    ExtensionsActionController(const modularity::ContextPtr& iocCtx)
        : Contextable(iocCtx) {}

    void init();

private:
    void registerExtensions();

    muse::Ret onExtensionTriggered(const rcommand::CommandQuery& q);
    void openUri(const UriQuery& uri, bool isSingle = true);

    std::shared_ptr<ExtensionsCommandsRegister> m_commandsRegister;
};
}
