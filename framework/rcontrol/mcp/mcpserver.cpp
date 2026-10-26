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

 #include "mcpserver.h"

 #include "serialization/json.h"

 #include "transport/tcptransport.h"
 #include "transport/httptransport.h"

 #include "log.h"

static const char* PROTOCOL_VERSION = "2025-11-25";
static const char* SERVER_NAME = "MuseMCP";

using namespace muse::rcontrol::mcp;

static muse::JsonValue toMcpJsonValue(const muse::Val& val)
{
    switch (val.type()) {
    case muse::Val::Type::Undefined: return muse::JsonValue();
    case muse::Val::Type::Bool: return muse::JsonValue(val.toBool());
    case muse::Val::Type::Int: return muse::JsonValue(val.toInt());
    case muse::Val::Type::Int64: return muse::JsonValue(static_cast<double>(val.toInt64()));
    case muse::Val::Type::Double: return muse::JsonValue(val.toDouble());
    case muse::Val::Type::String: return muse::JsonValue(val.toString());
    case muse::Val::Type::List: {
        muse::JsonArray array;
        for (const auto& item : val.toList()) {
            array.append(toMcpJsonValue(item));
        }
        return array;
    }
    case muse::Val::Type::Map: {
        muse::JsonObject object;
        for (const auto& [key, value] : val.toMap()) {
            object[key] = toMcpJsonValue(value);
        }
        return object;
    }
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

static std::map<std::string, muse::Val> toArgs(const muse::JsonObject& obj)
{
    std::map<std::string, muse::Val> args;
    for (const auto& key : obj.keys()) {
        args[key] = toVal(obj[key]);
    }
    return args;
}

McpServer::McpServer(const std::string& version, ITransport* transport)
    : m_version(version),
    m_transport(transport)
{
    if (!m_transport) {
        m_transport = new HttpTransport();
    }
}

McpServer::~McpServer()
{
    deinit();
    delete m_transport;
    m_transport = nullptr;
}

void McpServer::init()
{
    IF_ASSERT_FAILED(m_transport) {
        return;
    }

    m_transport->onRequest([this](const ByteArray& request, const ITransport::ResponseHandler& onResponse) {
        JsonObject req = JsonDocument::fromJson(request).rootObject();
        onRequest(req, [onResponse](const JsonObject& res) {
            JsonDocument doc(res);
            onResponse(doc.toJson(JsonDocument::Format::Compact));
        });
    });

    m_transport->start();
}

void McpServer::deinit()
{
    m_transport->stop();
}

void McpServer::onRequest(const JsonObject& request, const std::function<void(const JsonObject&)>& onResponse)
{
    const String method = request.value("method").toString();
    if (method == u"initialize") {
        onInitialize(request, onResponse);
    } else if (method == u"tools/list") {
        onToolsList(request, onResponse);
    } else if (method == u"tools/call") {
        onToolsCall(request, onResponse);
    } else if (method.startsWith(u"notifications/")) {
        onNotifications(method);
        onResponse(JsonObject());
    } else {
        LOGE() << "Unknown method: " << method;
        replyError(request, -32601, "Method not found", onResponse);
    }
}

void McpServer::onInitialize(const JsonObject& request, const std::function<void(const JsonObject&)>& onResponse)
{
    JsonObject response;
    response["jsonrpc"] = "2.0";
    response["id"] = request.value("id");

    JsonObject result;
    result["protocolVersion"] = PROTOCOL_VERSION;

    JsonObject toolCapabilities;
    toolCapabilities["listChanged"] = false;

    JsonObject capabilities;
    capabilities["tools"] = toolCapabilities;
    result["capabilities"] = capabilities;

    JsonObject serverInfo;
    serverInfo["name"] = SERVER_NAME;
    serverInfo["version"] = m_version;
    result["serverInfo"] = serverInfo;

    response["result"] = result;

    onResponse(response);
}

void McpServer::onToolsListRequest(const ToolsListHandler& onToolsList)
{
    m_onToolsList = onToolsList;
}

void McpServer::onToolsList(const JsonObject& request, const std::function<void(const JsonObject&)>& onResponse)
{
    if (!m_onToolsList) {
        replyError(request, -32603, "No tools list handler", onResponse);
        return;
    }

    const JsonValue id = request.value("id");
    m_onToolsList([id, onResponse](const std::vector<Tool>& tools) {
        JsonObject response;
        response["jsonrpc"] = "2.0";
        response["id"] = id;

        JsonArray arr;
        for (const Tool& tool : tools) {
            JsonObject obj;
            obj["name"] = tool.name;
            obj["title"] = tool.title;
            obj["description"] = tool.description;

            // input
            {
                const InputSchema& schema = tool.inputSchema;
                JsonObject schemaObj;
                schemaObj["type"] = schema.type;
                JsonObject properties;
                for (const auto& [name, property] : schema.properties) {
                    JsonObject propertyObj;
                    propertyObj["type"] = to_string(property.type);
                    propertyObj["description"] = property.description;
                    if (!property.minimum.isNull()) {
                        propertyObj["minimum"] = toMcpJsonValue(property.minimum);
                    }
                    if (!property.maximum.isNull()) {
                        propertyObj["maximum"] = toMcpJsonValue(property.maximum);
                    }
                    properties[name] = propertyObj;
                }
                schemaObj["properties"] = properties;
                obj["inputSchema"] = schemaObj;
            }

            // output
            {
                const OutputSchema& schema = tool.outputSchema;
                JsonObject schemaObj;
                schemaObj["type"] = schema.type;
                JsonObject properties;
                for (const auto& [name, property] : schema.properties) {
                    JsonObject propertyObj;
                    propertyObj["type"] = to_string(property.type);
                    propertyObj["description"] = property.description;
                    properties[name] = propertyObj;
                }
                schemaObj["properties"] = properties;
                obj["outputSchema"] = schemaObj;
            }

            arr << obj;
        }

        JsonObject result;
        result["tools"] = arr;

        response["result"] = result;

        onResponse(response);
    });
}

void McpServer::onToolsCallRequest(const ToolsCallHandler& onToolsCall)
{
    m_onToolsCall = onToolsCall;
}

void McpServer::onToolsCall(const JsonObject& request, const std::function<void(const JsonObject&)>& onResponse)
{
    if (!m_onToolsCall) {
        replyError(request, -32603, "No tools call handler", onResponse);
        return;
    }

    const JsonValue id = request.value("id");
    const JsonObject params = request.value("params").toObject();
    const std::string name = params.value("name").toString().toStdString();
    const JsonObject arguments = params.value("arguments").toObject();

    m_onToolsCall(name, toArgs(arguments), [id, onResponse](const ToolResult& toolResult) {
        JsonObject response;
        response["jsonrpc"] = "2.0";
        response["id"] = id;

        JsonObject resObj;
        JsonArray content;
        bool isError = false;

        if (toolResult.ret) {
            JsonObject structuredContent;
            for (const auto& [name, val] : toolResult.result) {
                structuredContent[name] = toMcpJsonValue(val);
            }
            resObj["structuredContent"] = structuredContent;

            JsonObject contentObj;
            contentObj["type"] = "text";
            contentObj["text"] = JsonDocument(structuredContent).toJson(JsonDocument::Format::Compact).constChar();
            content.append(contentObj);
        } else {
            isError = true;
            JsonObject errText;
            errText["type"] = "text";
            errText["text"] = toolResult.ret.toString();
            content.append(errText);
        }

        resObj["content"] = content;
        resObj["isError"] = isError;

        response["result"] = resObj;
        onResponse(response);
    });
}

void McpServer::onNotifications(const String& method)
{
    LOGI() << "Notification: " << method;
}

void McpServer::replyError(const JsonObject& request, int code, const std::string& message,
                           const std::function<void(const JsonObject&)>& onResponse)
{
    JsonObject error;
    error["code"] = code;
    error["message"] = message;

    JsonObject response;
    response["jsonrpc"] = "2.0";
    response["id"] = request.value("id");
    response["error"] = error;

    onResponse(response);
}
