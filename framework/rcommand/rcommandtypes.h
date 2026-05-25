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

#include <any>

#include "global/types/uri.h"
#include "global/types/ret.h"

namespace muse::rcommand {
using Command = Uri;
using CommandQuery = UriQuery;
using CallId = uint64_t;

struct Request
{
    CallId callId = 0;
    CommandQuery query;
};

struct Response
{
    CallId callId = 0;
    Ret ret;
    std::any data;

    template<typename T>
    T val() const
    {
        return std::any_cast<T>(data);
    }
};

inline CallId new_call_id()
{
    static CallId lastId = 0;
    ++lastId;
    return lastId;
}

inline Request make_request(const CommandQuery& query)
{
    Request request;
    request.callId = new_call_id();
    request.query = query;
    return request;
}

inline Response make_response(const Request& request, const Ret& ret)
{
    Response response;
    response.callId = request.callId;
    response.ret = ret;
    return response;
}

inline Response make_response(const Request& request, const Ret& ret, const std::any& data)
{
    Response response;
    response.callId = request.callId;
    response.ret = ret;
    response.data = data;
    return response;
}
}
