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

#include <any>
#include <map>
#include <string>

#include "global/types/uri.h"
#include "global/types/ret.h"
#include "global/types/string.h"
#include "global/types/val.h"
#include "global/types/mnemonicstring.h"
#include "global/types/translatablestring.h"
#include "global/types/color.h"
#include "ui/uitypes.h"

namespace muse::rcommand {
constexpr std::string_view COMMAND_SCHEME = "command://";
using Command = Uri;
using CommandQuery = UriQuery;

inline CommandQuery make_query(const std::string& command, const std::vector<std::pair<std::string, Val> >& params)
{
    CommandQuery query(command);
    for (const auto& param : params) {
        query.addParam(param.first, param.second);
    }
    return query;
}

// Info

enum class DataType {
    Undefined = 0,
    String,
    Integer,
    Float,
    Boolean,
    Object,
    Array,
    Null
};

struct Arg {
    DataType type = DataType::Undefined;
    String description;
    Val min;
    Val max;

    Arg() = default;
    Arg(DataType type, const String& description, const Val& min = Val(), const Val& max = Val())
        : type(type), description(description), min(min), max(max) {}
};

struct InputSchema {
    std::map<std::string, Arg> args;
};

enum class Checkable {
    No = 0,
    Yes
};

struct Decoration {
    ui::IconCode::Code iconCode = ui::IconCode::Code::NONE;
    Color iconColor;
    Checkable checkable = Checkable::No;

    Decoration() = default;
    Decoration(ui::IconCode::Code iconCode)
        : iconCode(iconCode) {}
    Decoration(ui::IconCode::Code iconCode, Color iconColor)
        : iconCode(iconCode), iconColor(iconColor) {}
    Decoration(ui::IconCode::Code iconCode, Checkable checkable)
        : iconCode(iconCode), checkable(checkable) {}
    Decoration(ui::IconCode::Code iconCode, Color iconColor, Checkable checkable)
        : iconCode(iconCode), iconColor(iconColor), checkable(checkable) {}
};

struct CommandInfo
{
    Command command;
    MnemonicString title;
    TranslatableString description;
    InputSchema inputSchema;
    Decoration decoration;

    bool isValid() const { return command.isValid(); }
};

struct CommandState {
    bool enabled = false;
    bool checked = false;

    CommandState() = default;
    CommandState(bool enabled, bool checked)
        : enabled(enabled), checked(checked) {}
    CommandState(bool enabled)
        : enabled(enabled), checked(false) {}

    bool operator==(const CommandState& other) const
    {
        return enabled == other.enabled && checked == other.checked;
    }

    bool operator!=(const CommandState& other) const
    {
        return !this->operator==(other);
    }
};

// Call

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
