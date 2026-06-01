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

#pragma once

#include <memory>

#include "modularity/ioc.h"
#include "global/iapplication.h"
#include "rcommand/icommanddispatcher.h"
#include "rcommand/icommandsregister.h"

#include "mcptypes.h"

namespace muse::rcontrol::mcp {
class McpServer;
class McpController : public Contextable
{
    GlobalInject<IApplication> application;
    GlobalInject<rcommand::ICommandsRegister> commandsRegister;
    ContextInject<rcommand::ICommandDispatcher> commandsDispatcher = { this };

public:
    McpController(const modularity::ContextPtr& iocCtx);
    ~McpController();

    void init();
    void deinit();

private:

    std::vector<Tool> makeToolsList() const;

    std::unique_ptr<McpServer> m_mcpServer;
};
}
