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

#include <cctype>
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

namespace {
constexpr std::string_view SERVER_NAME = "musescore-mcp-bridge";
constexpr std::string_view SERVER_VERSION = "0.1.0";

constexpr std::string_view SUPPORTED_PROTOCOL_VERSIONS[] = {
    "2025-11-25",
    "2025-03-26",
    "2024-11-05",
};

struct ToolStub {
    std::string_view name;
    std::string_view description;
    std::string_view inputSchema;
};

constexpr ToolStub TOOLS[] = {
    {
        "score_transpose",
        "Transpose the current score by a number of semitones (stub).",
        R"({"type":"object","properties":{"semitones":{"type":"integer","description":"Semitones to transpose"}},"required":["semitones"]})",
    },
    {
        "score_export_pdf",
        "Export the current score to PDF (stub).",
        R"({"type":"object","properties":{"outputPath":{"type":"string","description":"Output PDF path"}},"required":["outputPath"]})",
    },
    {
        "playback_toggle",
        "Toggle score playback (stub).",
        R"({"type":"object","properties":{}})",
    },
};

std::string escapeJson(std::string_view value)
{
    std::string escaped;
    escaped.reserve(value.size());

    for (char ch : value) {
        switch (ch) {
        case '"': escaped += "\\\"";
            break;
        case '\\': escaped += "\\\\";
            break;
        case '\n': escaped += "\\n";
            break;
        case '\r': escaped += "\\r";
            break;
        case '\t': escaped += "\\t";
            break;
        default: escaped += ch;
            break;
        }
    }

    return escaped;
}

std::string_view trimCr(std::string_view line)
{
    if (!line.empty() && line.back() == '\r') {
        line.remove_suffix(1);
    }
    return line;
}

std::string negotiateProtocolVersion(std::string_view clientVersion)
{
    if (!clientVersion.empty()) {
        for (const std::string_view version : SUPPORTED_PROTOCOL_VERSIONS) {
            if (clientVersion == version) {
                return std::string(version);
            }
        }
    }

    return std::string(SUPPORTED_PROTOCOL_VERSIONS[0]);
}

bool readStdioMessage(std::string& body)
{
    for (;;) {
        std::string line;
        if (!std::getline(std::cin, line)) {
            return false;
        }

        line = trimCr(line);
        if (!line.empty()) {
            body = std::move(line);
            return true;
        }
    }
}

void writeStdioMessage(std::string_view body)
{
    std::cout << body << '\n';
    std::cout.flush();
}

std::string extractJsonStringField(std::string_view json, std::string_view key)
{
    const std::string token = "\"" + std::string(key) + "\"";
    std::size_t pos = json.find(token);
    if (pos == std::string_view::npos) {
        return {};
    }

    pos = json.find(':', pos + token.size());
    if (pos == std::string_view::npos) {
        return {};
    }

    pos = json.find('"', pos);
    if (pos == std::string_view::npos) {
        return {};
    }

    const std::size_t valueStart = pos + 1;
    const std::size_t valueEnd = json.find('"', valueStart);
    if (valueEnd == std::string_view::npos) {
        return {};
    }

    return std::string(json.substr(valueStart, valueEnd - valueStart));
}

std::string extractRequestId(std::string_view json)
{
    const std::size_t pos = json.find("\"id\"");
    if (pos == std::string_view::npos) {
        return {};
    }

    const std::size_t colon = json.find(':', pos);
    if (colon == std::string_view::npos) {
        return {};
    }

    std::size_t valueStart = colon + 1;
    while (valueStart < json.size() && std::isspace(static_cast<unsigned char>(json[valueStart]))) {
        ++valueStart;
    }

    if (valueStart >= json.size()) {
        return {};
    }

    if (json[valueStart] == '"') {
        const std::size_t valueEnd = json.find('"', valueStart + 1);
        if (valueEnd == std::string_view::npos) {
            return {};
        }
        return std::string(json.substr(valueStart, valueEnd - valueStart + 1));
    }

    const std::size_t valueEnd = json.find_first_of(",}", valueStart);
    if (valueEnd == std::string_view::npos) {
        return {};
    }

    return std::string(json.substr(valueStart, valueEnd - valueStart));
}

std::string makeErrorResponse(std::string_view id, int code, std::string_view message)
{
    std::string response = R"({"jsonrpc":"2.0")";
    if (!id.empty()) {
        response += R"(,"id":)" + std::string(id);
    }
    response += R"(,"error":{"code":)" + std::to_string(code)
                + R"(,"message":")" + escapeJson(message) + R"("}})";
    return response;
}

std::string makeSuccessResponse(std::string_view id, std::string_view resultJson)
{
    return R"({"jsonrpc":"2.0","id":)" + std::string(id) + R"(,"result":)" + std::string(resultJson) + "}";
}

const ToolStub* findTool(std::string_view name)
{
    for (const ToolStub& tool : TOOLS) {
        if (tool.name == name) {
            return &tool;
        }
    }
    return nullptr;
}

std::string handleInitialize(std::string_view id, std::string_view request)
{
    const std::string protocolVersion = negotiateProtocolVersion(extractJsonStringField(request, "protocolVersion"));
    const std::string result = R"({"protocolVersion":")" + protocolVersion
                               + R"(","capabilities":{"tools":{"listChanged":false}},"serverInfo":{"name":")"
                               + escapeJson(SERVER_NAME) + R"(","version":")" + escapeJson(SERVER_VERSION)
                               + R"("}})";
    return makeSuccessResponse(id, result);
}

std::string handleToolsList(std::string_view id)
{
    std::string toolsJson = "[";

    for (std::size_t i = 0; i < sizeof(TOOLS) / sizeof(TOOLS[0]); ++i) {
        const ToolStub& tool = TOOLS[i];
        toolsJson += R"({"name":")" + escapeJson(tool.name)
                     + R"(","description":")" + escapeJson(tool.description)
                     + R"(","inputSchema":)" + std::string(tool.inputSchema) + "}";
        if (i + 1 != sizeof(TOOLS) / sizeof(TOOLS[0])) {
            toolsJson += ",";
        }
    }

    toolsJson += "]";
    return makeSuccessResponse(id, R"({"tools":)" + toolsJson + "}");
}

std::string handleToolsCall(std::string_view id, std::string_view request)
{
    const std::string toolName = extractJsonStringField(request, "name");
    if (toolName.empty()) {
        return makeErrorResponse(id, -32602, "Invalid params: missing tool name");
    }

    const ToolStub* tool = findTool(toolName);
    if (!tool) {
        return makeErrorResponse(id, -32602, "Unknown tool");
    }

    // Stub: real bridge will forward the call to MuseScore remote control.
    const std::string stubText = "[stub] Executed tool '" + toolName + "' successfully.";
    const std::string result = R"({"content":[{"type":"text","text":")" + escapeJson(stubText)
                               + R"("}],"isError":false})";
    return makeSuccessResponse(id, result);
}

std::string handlePing(std::string_view id)
{
    return makeSuccessResponse(id, "{}");
}

std::string dispatchRequest(std::string_view request)
{
    const std::string method = extractJsonStringField(request, "method");
    if (method.empty()) {
        return makeErrorResponse({}, -32600, "Invalid Request: missing method");
    }

    const std::string id = extractRequestId(request);
    const bool isNotification = id.empty();

    if (method == "notifications/initialized") {
        return {};
    }

    if (isNotification) {
        return {};
    }

    if (method == "initialize") {
        return handleInitialize(id, request);
    }

    if (method == "tools/list") {
        return handleToolsList(id);
    }

    if (method == "tools/call") {
        return handleToolsCall(id, request);
    }

    if (method == "ping") {
        return handlePing(id);
    }

    return makeErrorResponse(id, -32601, "Method not found");
}
} // namespace

int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    for (;;) {
        std::string request;
        if (!readStdioMessage(request)) {
            break;
        }

        if (request.empty()) {
            continue;
        }

        const std::string response = dispatchRequest(request);
        if (!response.empty()) {
            writeStdioMessage(response);
        }
    }

    return EXIT_SUCCESS;
}
