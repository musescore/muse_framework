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

using ErrorCode = std::pair<int, const char*>;

static constexpr ErrorCode PARSE_ERROR_CODE = { -32700, "Parse error" };
static constexpr ErrorCode INVALID_REQUEST_CODE = { -32600, "Invalid request" };
static constexpr ErrorCode METHOD_NOT_FOUND_CODE = { -32601, "Method not found" };
static constexpr ErrorCode INVALID_PARAMS_CODE = { -32602, "Invalid params" };
static constexpr ErrorCode INTERNAL_ERROR_CODE = { -32603, "Internal error" };

using namespace muse::rcontrol::mcp;

static std::map<std::string, muse::Val> toArgs(const muse::JsonObject& obj)
{
    std::map<std::string, muse::Val> args;
    for (const auto& key : obj.keys()) {
        args[key] = obj[key].toVal();
    }
    return args;
}

static void replyError(const muse::JsonObject& request, const ErrorCode& errorCode,
                       const std::function<void(const muse::JsonObject&)>& onResponse)
{
    muse::JsonObject error;
    error["code"] = errorCode.first;
    error["message"] = errorCode.second;

    muse::JsonObject response;
    response["jsonrpc"] = "2.0";
    response["id"] = request.value("id");
    response["error"] = error;

    onResponse(response);
}

static void replyError(const muse::JsonObject& request, const ErrorCode& errorCode,
                       const ITransport::ResponseHandler& onResponse)
{
    replyError(request, errorCode, [onResponse](const muse::JsonObject& res) {
        onResponse(muse::JsonDocument(res).toJson(muse::JsonDocument::Format::Compact));
    });
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
        std::string err;
        JsonDocument json = JsonDocument::fromJson(request, &err);

        if (!err.empty()) {
            LOGE() << "Parse error: " << err;
            replyError(JsonObject(), PARSE_ERROR_CODE, onResponse);
            return;
        }

        if (!json.isObject()) {
            LOGE() << "Invalid request: not an object";
            replyError(JsonObject(), INVALID_REQUEST_CODE, onResponse);
            return;
        }

        JsonObject req = json.rootObject();
        onRequest(req, [onResponse](const JsonObject& res) {
            JsonDocument doc(res);
            ByteArray json = doc.toJson(JsonDocument::Format::Compact);
            onResponse(json);
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
    if (!request.contains("method")) {
        replyError(request, INVALID_REQUEST_CODE, onResponse);
        return;
    }

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
        replyError(request, METHOD_NOT_FOUND_CODE, onResponse);
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
        replyError(request, INTERNAL_ERROR_CODE, onResponse);
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

            auto propertiesSchema = [](const PropertiesSchema& schema) {
                JsonObject schemaObj;
                schemaObj["type"] = schema.type;
                JsonObject properties;
                for (const auto& [name, property] : schema.properties) {
                    JsonObject propertyObj;
                    propertyObj["type"] = to_string(property.type.type);
                    if (property.type.format != Format::Undefined) {
                        propertyObj["format"] = to_string(property.type.format);
                    }
                    propertyObj["description"] = property.description;

                    if (property.enumValues) {
                        propertyObj["enum"] = JsonArray::fromValList(*property.enumValues);
                    }
                    if (property.minimum) {
                        propertyObj["minimum"] = JsonValue::fromVal(*property.minimum);
                    }
                    if (property.maximum) {
                        propertyObj["maximum"] = JsonValue::fromVal(*property.maximum);
                    }
                    properties[name] = propertyObj;
                }

                if (!properties.empty()) {
                    schemaObj["properties"] = properties;

                    if (!schema.required.empty()) {
                        schemaObj["required"] = JsonArray::fromStringList(schema.required);
                    }
                }

                return schemaObj;
            };

            obj["inputSchema"] = propertiesSchema(tool.inputSchema);

            if (!tool.outputSchema.properties.empty()) {
                obj["outputSchema"] = propertiesSchema(tool.outputSchema);
            }

            JsonObject annotations;
            annotations["readOnlyHint"] = tool.annotations.readOnly;
            annotations["destructiveHint"] = tool.annotations.destructive;
            annotations["idempotentHint"] = tool.annotations.idempotent;
            annotations["openWorldHint"] = tool.annotations.openWorld;
            obj["annotations"] = annotations;

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
        replyError(request, INTERNAL_ERROR_CODE, onResponse);
        return;
    }

    if (!request.contains("id")) {
        replyError(request, INVALID_REQUEST_CODE, onResponse);
        return;
    }

    if (!(request.contains("params") && request.value("params").isObject())) {
        replyError(request, INVALID_PARAMS_CODE, onResponse);
        return;
    }

    const JsonValue id = request.value("id");
    const JsonObject params = request.value("params").toObject();

    if (!(params.contains("name") && params.value("name").isString())) {
        replyError(request, INVALID_PARAMS_CODE, onResponse);
        return;
    }

    const std::string name = params.value("name").toString().toStdString();
    JsonObject arguments;
    if (params.contains("arguments") && !params.value("arguments").isNull()) {
        if (params.value("arguments").isObject()) {
            arguments = params.value("arguments").toObject();
        } else {
            replyError(request, INVALID_PARAMS_CODE, onResponse);
            return;
        }
    }

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
                structuredContent[name] = JsonValue::fromVal(val);
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
