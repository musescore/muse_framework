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
 #include "thirdparty/kors_logger/src/log_base.h"
 #include "transport/tcptransport.h"

 #include "log.h"

static const char* PROTOCOL_VERSION = "2025-11-25";
static const char* SERVER_NAME = "MuseMCP";

using namespace muse::rcontrol::mcp;

McpServer::McpServer(const std::string& version, ITransport* transport)
    : m_version(version),
    m_transport(transport)
{
    if (!m_transport) {
        m_transport = new TcpTransport();
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

            const InputSchema& schema = tool.inputSchema;
            JsonObject schemaObj;
            schemaObj["type"] = schema.type;
            for (const auto& property : schema.properties) {
                JsonObject propertyObj;
                propertyObj["type"] = to_string(property.second.type);
                propertyObj["description"] = property.second.description;
                propertyObj["minimum"] = property.second.minimum.toString();
                propertyObj["maximum"] = property.second.maximum.toString();
                schemaObj[property.first] = propertyObj;
            }
            obj["inputSchema"] = schemaObj;

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

    m_onToolsCall(name, arguments, [id, onResponse](const ToolResult& toolResult) {
        JsonObject response;
        response["jsonrpc"] = "2.0";
        response["id"] = id;

        JsonObject resObj;
        resObj["content"] = JsonArray();
        resObj["isError"] = toolResult.isError;

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
