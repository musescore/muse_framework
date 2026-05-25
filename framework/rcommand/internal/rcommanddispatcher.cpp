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
#include "rcommanddispatcher.h"

#include "async/promise.h"

#include "../rcommandable.h"
#include "../rcommandtypes.h"

#include "log.h"

using namespace muse;
using namespace muse::rcommand;

async::Promise<Response> RCommandDispatcher::dispatch(const Request& request)
{
    return async::make_promise<Response>([this, request](auto resolve) {
        auto it = m_clients.find(Command(request.query.uri()));
        if (it != m_clients.end()) {
            Response response = it->second.callback(request);
            return resolve(response);
        } else {
            return resolve(make_response(request, make_ret(Ret::Code::UnknownError)));
        }
    });
}

void RCommandDispatcher::onRequest(RCommandable* client, const Command& command, const CallBack& callback)
{
    IF_ASSERT_FAILED(m_clients.find(command) == m_clients.end()) {
        LOGW() << "command already registered: " << command;
        return;
    }

    m_clients[command] = { client, callback };
    client->setDispatcher(this);
}

void RCommandDispatcher::unreg(RCommandable* client)
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
