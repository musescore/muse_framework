/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore/Audacity CLA applies
 *
 * Copyright (C) MuseScore/Audacity and others
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

#include "modularity/imoduleinterface.h"

#include "global/async/promise.h"

#include "commandtypes.h"

namespace muse::rcommand {
class Commandable;
class ICommandDispatcher : MODULE_CONTEXT_INTERFACE
{
    INTERFACE_ID(ICommandDispatcher)
public:
    virtual ~ICommandDispatcher() = default;

    using CallBack = std::function<Response (const Request& request)>;
    using CallBackRet = std::function<Ret ()>;

    virtual async::Promise<Response> dispatch(const Request& request) = 0;
    virtual void onRequest(Commandable* client, const Command& command, const CallBack& callback) = 0;
    virtual void unreg(Commandable* client) = 0;

    // Helpers for convenience

    async::Promise<Response> dispatch(const CommandQuery& query)
    {
        return dispatch(make_request(query));
    }

    async::Promise<Response> dispatch(const Command& query)
    {
        return dispatch(CommandQuery(query));
    }

    void onRequest(Commandable* client, const Command& command, const CallBackRet& callback)
    {
        onRequest(client, command, [callback](const Request& request) {
            return make_response(request, callback());
        });
    }
};
}
