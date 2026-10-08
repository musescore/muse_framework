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
#include "global/async/channel.h"
#include "global/types/ret.h"

#include "commandtypes.h"
#include "typedcommand.h"

namespace muse::rcommand {
class Commandable;
class ICommandDispatcher : MODULE_CONTEXT_INTERFACE
{
    INTERFACE_ID(ICommandDispatcher)
public:
    virtual ~ICommandDispatcher() = default;

    using OnResponse = std::function<void (const Response& response)>;

    using CallBack = std::function<Response (const Request& request)>;
    using AsyncCallBack = std::function<void (const Request& request, OnResponse onResponse)>;

    using CallBackRet = std::function<Ret ()>;
    using CallBackParamsRet = std::function<Ret (const Params& params)>;
    using CallBackParamsPromiseRet = std::function<async::Promise<Ret>(const Params& params)>;

    virtual async::Promise<Response> dispatch(const Request& request) = 0;
    virtual void onRequest(Commandable* client, const Command& command, const CallBack& callback) = 0;
    virtual void onRequest(Commandable* client, const Command& command, const AsyncCallBack& callback) = 0;
    virtual void unreg(Commandable* client) = 0;

    virtual async::Channel<Command, bool* /*allow dispatch*/> preDispatch() const = 0;
    virtual async::Channel<Command> postDispatch() const = 0;

    // Helpers for convenience
    async::Promise<Response> dispatch(const Command& command)
    {
        return dispatch(make_request(command, {}));
    }

    async::Promise<Response> dispatch(const Command& command, const Params& params)
    {
        return dispatch(make_request(command, params));
    }

    async::Promise<Response> dispatch(const CommandQuery& query)
    {
        return dispatch(make_request(query.uri(), query.params()));
    }

    void onRequest(Commandable* client, const Command& command, const CallBackRet& callback)
    {
        onRequest(client, command, [callback](const Request& request) {
            return make_response(request, callback());
        });
    }

    void onRequest(Commandable* client, const Command& command, const CallBackParamsRet& callback)
    {
        onRequest(client, command, [callback](const Request& request) {
            return make_response(request, callback(request.params));
        });
    }

    void onRequest(Commandable* client, const Command& command, const CallBackParamsPromiseRet& callback)
    {
        onRequest(client, command, AsyncCallBack([callback](const Request& request, const OnResponse& onResponse) {
            answerWhenResolved(request, callback(request.params), onResponse);
        }));
    }

    // Typed commands (see typedcommand.h): the struct is the parameter list

    template<typename C, std::enable_if_t<IsTypedCommand<C>::value, int> = 0>
    async::Promise<Response> dispatch(const C& command)
    {
        return dispatch(make_request(C::id, toParams(command)));
    }

    //! The callback receives the command struct; Params that do not match it
    //! (missing, mistyped or unknown parameters) are answered with BadArgs
    template<typename C, std::enable_if_t<IsTypedCommand<C>::value, int> = 0>
    void onRequest(Commandable* client, std::function<Ret(const C&)> callback)
    {
        onRequest(client, C::id, CallBack([callback](const Request& request) {
            C command;
            std::string err;
            if (!fromParams(request.params, command, err)) {
                return make_response(request, make_ret(Ret::Code::BadArgs, err));
            }
            return make_response(request, callback(command));
        }));
    }

private:

    static void answerWhenResolved(const Request& request, async::Promise<Ret> retPromise, const OnResponse& onResponse)
    {
        retPromise.onResolve(nullptr, [request, onResponse](const Ret& ret) {
            onResponse(make_response(request, ret));
        });
    }
};
}
