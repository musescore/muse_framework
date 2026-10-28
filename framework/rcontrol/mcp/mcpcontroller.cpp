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

static TypeSchema toMcpTypeSchema(muse::rcommand::DataType type)
{
    using CT = muse::rcommand::DataType;
    using MT = muse::rcontrol::mcp::DataType;
    using MF = muse::rcontrol::mcp::Format;

    switch (type) {
    case CT::Undefined: return TypeSchema{ MT::Undefined, MF::Undefined };
    case CT::String: return TypeSchema{ MT::String, MF::Undefined };
    case CT::Int: return TypeSchema{ MT::Integer, MF::Undefined };
    case CT::Int64: return TypeSchema{ MT::Integer, MF::Int64 };
    case CT::Float: return TypeSchema{ MT::Number, MF::Float };
    case CT::Double: return TypeSchema{ MT::Number, MF::Double };
    case CT::Bool: return TypeSchema{ MT::Boolean, MF::Undefined };
    case CT::List: return TypeSchema{ MT::Array, MF::Undefined };
    case CT::Map: return TypeSchema{ MT::Object, MF::Undefined };
    }
    return TypeSchema{ MT::Undefined, MF::Undefined };
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
            property.type = toMcpTypeSchema(arg.type);
            property.description = arg.description.toStdString();
            property.minimum = arg.min;
            property.maximum = arg.max;
            tool.inputSchema.properties[name] = property;
        }

        for (const auto& [name, arg] : info.outputSchema.args) {
            Property property;
            property.type = toMcpTypeSchema(arg.type);
            property.description = arg.description.toStdString();
            tool.outputSchema.properties[name] = property;
        }
        tools.push_back(std::move(tool));
    }
    return tools;
}
