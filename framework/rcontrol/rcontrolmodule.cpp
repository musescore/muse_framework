/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore/Audacity CLA applies
 *
 * Copyright (C) 2026 MuseScore/Audacity and others
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

#include "rcontrolmodule.h"

#include "mcp/mcpcontroller.h"

using namespace muse;
using namespace muse::rcontrol;

static const std::string mname("rcontrol");

std::string RControlModule::moduleName() const
{
    return mname;
}

modularity::IContextSetup* RControlModule::newContext(const muse::modularity::ContextPtr& ctx) const
{
    return new RControlContext(ctx);
}

void RControlContext::registerExports()
{
    m_mcpController = std::make_shared<mcp::McpController>(iocContext());
}

void RControlContext::onInit(const IApplication::RunMode& mode)
{
    if (mode != IApplication::RunMode::GuiApp) {
        return;
    }

    m_mcpController->init();
}

void RControlContext::onDeinit()
{
    m_mcpController->deinit();
}
