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
#include "commanddispatcher.h"

#include "async/promise.h"

#include "../commandable.h"
#include "../commandtypes.h"

#include "log.h"

using namespace muse;
using namespace muse::rcommand;

CommandDispatcher::~CommandDispatcher()
{
    for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
        Commandable* client = it->second.client;
        if (client) {
            client->setDispatcher(nullptr);
        }
    }

    m_clients.clear();
}

async::Promise<Response> CommandDispatcher::dispatch(const Request& request)
{
    return async::make_promise<Response>([this, request](auto resolve) {
        auto it = m_clients.find(Command(request.query.uri()));
        if (it != m_clients.end()) {
            LOGI() << "try call command query: " << request.query.toString();
            Response response = it->second.callback(request);
            return resolve(response);
        } else {
            return resolve(make_response(request, make_ret(Ret::Code::UnknownError)));
        }
    });
}

void CommandDispatcher::onRequest(Commandable* client, const Command& command, const CallBack& callback)
{
    IF_ASSERT_FAILED(m_clients.find(command) == m_clients.end()) {
        LOGW() << "command already registered: " << command;
        return;
    }

    m_clients[command] = { client, callback };
    client->setDispatcher(this);
}

void CommandDispatcher::unreg(Commandable* client)
{
    if (!client || !client->isDispatcher(this)) {
        return;
    }

    for (auto it = m_clients.begin(); it != m_clients.end();) {
        if (it->second.client == client) {
            it = m_clients.erase(it);
            continue;
        }
        ++it;
    }

    client->setDispatcher(nullptr);
}
