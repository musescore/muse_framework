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

 #include <functional>
 #include <string>
 #include <vector>

 #include "itransport.h"
 #include "serialization/json.h"
 #include "mcptypes.h"

namespace muse::rcontrol::mcp {
class McpServer
{
public:

    McpServer(const std::string& version, ITransport* transport = nullptr);
    ~McpServer();

    void init();
    void deinit();

    using ToolsListResultHandler = std::function<void (std::vector<Tool>)>;
    using ToolsListHandler = std::function<void (ToolsListResultHandler)>;
    using ToolsCallResultHandler = std::function<void (const ToolResult&)>;
    using ToolsCallHandler = std::function<void (const std::string& name,
                                                 const JsonObject& arguments,
                                                 ToolsCallResultHandler)>;

    void onToolsListRequest(const ToolsListHandler& onToolsList);
    void onToolsCallRequest(const ToolsCallHandler& onToolsCall);

private:

    void onRequest(const JsonObject& request, const std::function<void(const JsonObject&)>& onResponse);

    void onInitialize(const JsonObject& request, const std::function<void(const JsonObject&)>& onResponse);
    void onToolsList(const JsonObject& request, const std::function<void(const JsonObject&)>& onResponse);
    void onToolsCall(const JsonObject& request, const std::function<void(const JsonObject&)>& onResponse);
    void onNotifications(const String& method);

    void replyError(const JsonObject& request, int code, const std::string& message,
                    const std::function<void(const JsonObject&)>& onResponse);

    std::string m_version;
    ITransport* m_transport = nullptr;
    ToolsListHandler m_onToolsList;
    ToolsCallHandler m_onToolsCall;
};
}
