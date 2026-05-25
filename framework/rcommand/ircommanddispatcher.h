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

#include "modularity/imoduleinterface.h"

#include "global/async/promise.h"

#include "rcommandtypes.h"

namespace muse::rcommand {
class RCommandable;
class IRCommandDispatcher : MODULE_CONTEXT_INTERFACE
{
    INTERFACE_ID(IRCommandDispatcher)
public:
    virtual ~IRCommandDispatcher() = default;

    using CallBack = std::function<Response (const Request& request)>;

    virtual async::Promise<Response> dispatch(const Request& request) = 0;
    virtual void onRequest(RCommandable* client, const Command& command, const CallBack& callback) = 0;
    virtual void unreg(RCommandable* client) = 0;

    async::Promise<Response> dispatch(const CommandQuery& query)
    {
        return dispatch(make_request(query));
    }
};
}
