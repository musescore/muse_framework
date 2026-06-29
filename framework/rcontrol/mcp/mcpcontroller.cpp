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

#include "global/stringutils.h"
#include "global/serialization/json.h"

#include "log.h"
#include "thirdparty/kors_logger/src/log_base.h"

using namespace muse::rcontrol::mcp;
using namespace muse::rcommand;

McpController::McpController(const modularity::ContextPtr& iocCtx)
    : Contextable(iocCtx)
{
}

McpController::~McpController()
{
    deinit();
}

//! NOTE The tool name must be in the format: group_name
static std::string commandToToolName(const Command& command)
{
    std::string path = command.path();
    muse::strings::replace(path, "/", "_");
    return path;
}

static CommandQuery commandQuery(const std::string& name, const muse::JsonObject& args)
{
    UNUSED(args); // TODO: implement
    std::string path = name;
    muse::strings::replace(path, "_", "/");
    Command cmd(std::string(COMMAND_SCHEME), path);
    CommandQuery q(cmd);
    // for (const auto& arg : args) {
    //     q.set(arg.first, arg.second.toString());
    // }
    return q;
}

void McpController::init()
{
    m_mcpServer = std::make_unique<McpServer>(application()->version().toStdString());

    m_mcpServer->onToolsListRequest([this](const McpServer::ToolsListResultHandler& onResult) {
        std::vector<Tool> tools = makeToolsList();
        onResult(tools);
    });

    m_mcpServer->onToolsCallRequest([this](const std::string& name,
                                           const muse::JsonObject& args,
                                           const McpServer::ToolsCallResultHandler& onResult)
    {
        LOGDA() << "Tools call: " << name;

        commandsDispatcher()->dispatch(commandQuery(name, args));

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
    std::vector<Tool> tools;
    auto commandList = commandsRegister()->commandInfoList();
    tools.reserve(commandList.size());
    for (const auto& info : commandList) {
        Tool tool;
        tool.name = commandToToolName(info.command);
        tool.title = info.title.raw().translated().toStdString();
        tool.description = info.description.translated().toStdString();
        tool.inputSchema = InputSchema();
        // for (const auto& arg : info.inputSchema.args) {
        //     Property property;
        //     property.name = String::fromStdString(arg.first);
        //     property.type = String::fromStdString(arg.second.type);
        //     property.description = String::fromStdString(arg.second.description);
        //     property.minimum = String::fromStdString(arg.second.minimum);
        //     property.maximum = String::fromStdString(arg.second.maximum);
        // }
        tools.push_back(std::move(tool));
    }
    return tools;
}
