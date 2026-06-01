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

#include "rcommandmodule.h"

#include "internal/commandsregister.h"
#include "internal/commanddispatcher.h"

using namespace muse;
using namespace muse::rcommand;

static const std::string mname("rcommand");

std::string RCommandModule::moduleName() const
{
    return mname;
}

void RCommandModule::registerExports()
{
    globalIoc()->registerExport<ICommandsRegister>(mname, new CommandsRegister());
}

modularity::IContextSetup* RCommandModule::newContext(const muse::modularity::ContextPtr& ctx) const
{
    return new RCommandContext(ctx);
}

void RCommandContext::registerExports()
{
    ioc()->registerExport<ICommandDispatcher>(mname, new CommandDispatcher());
}
