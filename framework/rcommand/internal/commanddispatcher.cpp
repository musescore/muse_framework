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
    auto it = m_clients.find(request.command);
    if (it == m_clients.end()) {
        LOGW() << "command not registered: " << request.command;
        return async::make_promise<Response>([request](auto resolve) {
            return resolve(make_response(request, make_ret(Ret::Code::UnknownError)));
        });
    }

    LOGI() << "try call command: " << request.command << ", params: " << request.params;

    Client client = it->second;

    return async::make_promise<Response>([request, client](auto resolve) {
        if (client.asyncCallback) {
            client.asyncCallback(request, [resolve](const Response& response) {
                (void)resolve(response);
            });

            return async::Promise<Response>::dummy_result();
        }

        if (client.callback) {
            return resolve(client.callback(request));
        }

        UNREACHABLE;
        return resolve(make_response(request, make_ret(Ret::Code::UnknownError)));
    });
}

Response CommandDispatcher::dispatch(const Command& command, const Params& params)
{
    Request request = make_request(command, params);

    auto it = m_clients.find(command);
    if (it == m_clients.end()) {
        LOGW() << "command not registered: " << command;
        return make_response(request, make_ret(Ret::Code::UnknownError));
    }

    LOGI() << "try call command: " << command << " with params: " << params;

    CallBack callback = it->second.callback;

    IF_ASSERT_FAILED(callback) {
        return make_response(request, make_ret(Ret::Code::NotSupported));
    }

    return callback(request);
}

Response CommandDispatcher::dispatch(const CommandQuery& query)
{
    return dispatch(query.uri(), query.params());
}

void CommandDispatcher::onRequest(Commandable* client, const Command& command, const CallBack& callback)
{
    reg(client, command, { client, callback, nullptr });
}

void CommandDispatcher::onRequest(Commandable* client, const Command& command, const AsyncCallBack& callback)
{
    reg(client, command, { client, nullptr, callback });
}

void CommandDispatcher::reg(Commandable* client, const Command& command, const Client& c)
{
    IF_ASSERT_FAILED(m_clients.find(command) == m_clients.end()) {
        LOGW() << "command already registered: " << command;
        return;
    }

    m_clients[command] = c;
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
