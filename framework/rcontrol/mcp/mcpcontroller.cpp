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

static muse::JsonValue toMcpJsonValue(const muse::Val& val)
{
    switch (val.type()) {
    case muse::Val::Type::Undefined: return muse::JsonValue();
    case muse::Val::Type::Bool: return muse::JsonValue(val.toBool());
    case muse::Val::Type::Int: return muse::JsonValue(val.toInt());
    case muse::Val::Type::Int64: return muse::JsonValue(val.toInt());
    case muse::Val::Type::Double: return muse::JsonValue(val.toDouble());
    case muse::Val::Type::String: return muse::JsonValue(val.toString());
    default: {
        NOT_SUPPORTED << "data type: " << (int)val.type();
    }
    }

    return muse::JsonValue();
}

static muse::Val toVal(const muse::JsonValue& val)
{
    if (val.isNull()) {
        return muse::Val();
    }
    if (val.isBool()) {
        return muse::Val(val.toBool());
    }
    if (val.isNumber()) {
        return muse::Val(val.toDouble());
    }
    if (val.isString()) {
        return muse::Val(val.toString().toStdString());
    }
    if (val.isArray()) {
        muse::ValList list;
        const muse::JsonArray array = val.toArray();
        for (size_t i = 0; i < array.size(); ++i) {
            list.emplace_back(toVal(array[i]));
        }
        return muse::Val(list);
    }
    if (val.isObject()) {
        muse::ValMap map;
        const muse::JsonObject object = val.toObject();
        for (const auto& key : object.keys()) {
            map[key] = toVal(object[key]);
        }
        return muse::Val(map);
    }

    return muse::Val();
}

static muse::rcommand::Params toParams(const muse::JsonObject& obj)
{
    muse::rcommand::Params params;
    for (const auto& key : obj.keys()) {
        params[key] = toVal(obj[key]);
    }
    return params;
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
                                           const muse::JsonObject& args,
                                           const McpServer::ToolsCallResultHandler& onResult)
    {
        LOGDA() << "Tools call: " << name;

        async::Promise<Response> promise = commandsDispatcher()->dispatch(toolNameToCommand(name), toParams(args));
        promise.onResolve(this, [onResult](const Response& response) {
            ToolResult result;
            result.ret = response.ret;
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
            property.minimum = toMcpJsonValue(arg.min);
            property.maximum = toMcpJsonValue(arg.max);
            tool.inputSchema.properties[name] = property;
        }
        tools.push_back(std::move(tool));
    }
    return tools;
}
