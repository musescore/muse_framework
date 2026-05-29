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

#include "mcpcontroller.h"

#include "mcpserver.h"

#include "log.h"

using namespace muse::rcontrol::mcp;

McpController::McpController(const modularity::ContextPtr& iocCtx)
    : Contextable(iocCtx)
{
}

McpController::~McpController()
{
    deinit();
}

void McpController::init()
{
    m_mcpServer = std::make_unique<McpServer>(application()->version().toStdString());

    m_mcpServer->onToolsListRequest([this](const McpServer::ToolsListResultHandler& onResult) {
        std::vector<Tool> tools = makeToolsList();
        onResult(tools);
    });

    m_mcpServer->onToolsCallRequest([this](const std::string& name,
                                           const JsonObject& args,
                                           const McpServer::ToolsCallResultHandler& onResult)
    {
        LOGDA() << "Tools call: " << name;

        dispatcher()->dispatch(name);

        onResult(ToolResult());
    });

    m_mcpServer->init();
}

void McpController::deinit()
{
    if (m_mcpServer) {
        m_mcpServer->deinit();
        m_mcpServer = nullptr;
    }
}

std::vector<Tool> McpController::makeToolsList() const
{
    //! NOTE There will be an adapter to the RCommand infrastructure.
    std::vector<Tool> tools;
    tools.push_back({ String(u"play"), String(u"Play"), String(u"Start playback of the current score"), InputSchema() });
    tools.push_back({ String(u"stop"), String(u"Stop"), String(u"Stop playback of the current score"), InputSchema() });
    return tools;
}
