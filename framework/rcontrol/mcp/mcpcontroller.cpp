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
using namespace muse::rcommand;

static muse::rcontrol::mcp::DataType toMcpDataType(muse::rcommand::DataType type)
{
    switch (type) {
    case muse::rcommand::DataType::Undefined: return muse::rcontrol::mcp::DataType::Undefined;
    case muse::rcommand::DataType::String: return muse::rcontrol::mcp::DataType::String;
    case muse::rcommand::DataType::Integer: return muse::rcontrol::mcp::DataType::Integer;
    case muse::rcommand::DataType::Float: return muse::rcontrol::mcp::DataType::Float;
    case muse::rcommand::DataType::Boolean: return muse::rcontrol::mcp::DataType::Boolean;
    case muse::rcommand::DataType::Object: return muse::rcontrol::mcp::DataType::Object;
    case muse::rcommand::DataType::Array: return muse::rcontrol::mcp::DataType::Array;
    case muse::rcommand::DataType::Null: return muse::rcontrol::mcp::DataType::Null;
    }
    return muse::rcontrol::mcp::DataType::Undefined;
}

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
    std::replace(path.begin(), path.end(), '/', '.');
    return path;
}

static Command toolNameToCommand(const std::string& name)
{
    std::string path = name;
    std::replace(path.begin(), path.end(), '.', '/');
    return Command(std::string(COMMAND_SCHEME), path);
}

void McpController::init()
{
    m_mcpServer = std::make_unique<McpServer>(application()->version().toStdString());

    m_mcpServer->onToolsListRequest([this](const McpServer::ToolsListResultHandler& onResult) {
        std::vector<Tool> tools = makeToolsList();
        onResult(tools);
    });

    m_mcpServer->onToolsCallRequest([this](const std::string& name,
                                           const std::map<std::string, Val>& args,
                                           const McpServer::ToolsCallResultHandler& onResult)
    {
        LOGDA() << "Tools call: " << name;

        async::Promise<Response> promise = commandsDispatcher()->dispatch(toolNameToCommand(name), args);
        promise.onResolve(this, [onResult](const Response& response) {
            ToolResult result;
            result.ret = response.ret;
            result.result = response.result;
            onResult(result);
        });
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
        if (!info.availabilities.testFlag(Availability::MCP)) {
            continue;
        }
        Tool tool;
        tool.name = commandToToolName(info.command);
        tool.title = info.title.raw().translated().toStdString();
        tool.description = info.description.translated().toStdString();
        for (const auto& [name, arg] : info.inputSchema.args) {
            Property property;
            property.type = toMcpDataType(arg.type);
            property.description = arg.description.toStdString();
            property.minimum = arg.min;
            property.maximum = arg.max;
            tool.inputSchema.properties[name] = property;
        }

        for (const auto& [name, arg] : info.outputSchema.args) {
            Property property;
            property.type = toMcpDataType(arg.type);
            property.description = arg.description.toStdString();
            tool.outputSchema.properties[name] = property;
        }
        tools.push_back(std::move(tool));
    }
    return tools;
}
