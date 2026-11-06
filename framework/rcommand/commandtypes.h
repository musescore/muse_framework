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

#include <map>
#include <string>

#include "global/types/uri.h"
#include "global/types/ret.h"
#include "global/types/string.h"
#include "global/types/val.h"
#include "global/types/mnemonicstring.h"
#include "global/types/translatablestring.h"
#include "global/types/color.h"
#include "global/types/flags.h"
#include "global/logstream.h"
#include "ui/uitypes.h"

namespace muse::rcommand {
constexpr std::string_view COMMAND_SCHEME = "command";
using Command = Uri;
using CommandQuery = UriQuery;

//! NOTE This is a valid command that does nothing.
inline static const Command NONE_COMMAND = Command("command://none");

struct Params : public std::map<std::string, Val, std::less<> >
{
    Params() = default;
    Params(const std::map<std::string, Val>& params)
        : std::map<std::string, Val, std::less<> >(params.begin(), params.end()) {}

    Params(std::initializer_list<value_type> params)
        : std::map<std::string, Val, std::less<> >(params.begin(), params.end()) {}

    operator std::map<std::string, Val>() const
    {
        return std::map<std::string, Val>(begin(), end());
    }

    bool contains(const std::string& key) const
    {
        return find(key) != end();
    }

    Params& set(const std::string& key, const Val& value)
    {
        (*this)[key] = value;
        return *this;
    }

    Params& set(const std::string_view& key, const Val& value)
    {
        return set(std::string(key), value);
    }

    Params& set(const char* key, const Val& value)
    {
        return set(std::string(key), value);
    }

    Params& set(const std::pair<std::string, Val>& pair)
    {
        return set(pair.first, pair.second);
    }

    // hides the std::map implementation of at()
    Val at(const std::string& key, const Val& def = Val()) const
    {
        auto it = find(key);
        return it != end() ? it->second : def;
    }

    Val at(const std::string_view& key, const Val& def = Val()) const
    {
        auto it = find(key);
        return it != end() ? it->second : def;
    }

    Val at(const char* key, const Val& def = Val()) const
    {
        return at(std::string_view(key), def);
    }
};

inline CommandQuery make_query(const Command& command, const Params& params)
{
    CommandQuery query(command);
    query.setParams(params);
    return query;
}

inline CommandQuery make_query(const std::string& command, const Params& params)
{
    return make_query(Command(command), params);
}

// Info

enum class DataType {
    Undefined = 0,
    Bool,
    Int,
    Int64,
    Float,
    Double,
    String,
    Enum,
    List,
    Map
};

enum class Presence {
    Required,
    Optional,
};

struct Arg {
    std::string description;
    DataType dataType = DataType::Undefined;
    Presence presence = Presence::Required;
    std::optional<ValList> allowedValues; //! NOTE Typically, these are enum values.
    std::optional<Val> min;
    std::optional<Val> max;

    Arg() = default;

    Arg(DataType dataType, const String& description,
        const std::optional<Val>& min = std::nullopt, const std::optional<Val>& max = std::nullopt)
        : description(description.toStdString()), dataType(dataType), min(min), max(max) {}

    Arg(DataType dataType, const std::string& description, Presence presence,
        const std::optional<Val>& min = std::nullopt, const std::optional<Val>& max = std::nullopt)
        : description(description), dataType(dataType), presence(presence), min(min), max(max) {}

    Arg(DataType dataType, const std::string& description,
        const std::optional<Val>& min = std::nullopt, const std::optional<Val>& max = std::nullopt)
        : description(description), dataType(dataType), min(min), max(max) {}
};

struct ArgsSchema {
    ArgsSchema() {}
    ArgsSchema(std::map<std::string, Arg> _args)
        : args(std::move(_args)) {}

    std::map<std::string, Arg> args;
};

using InputSchema = ArgsSchema;
using OutputSchema = ArgsSchema;

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
    Decoration(Checkable checkable)
        : checkable(checkable) {}
};

enum class Availability {
    Disabled = 0,
    Internal = 1 << 0,
    Shortcut = 1 << 1,
    Extension = 1 << 2,
    MCP = 1 << 3,

    UI = Internal | Shortcut,
    API = Extension | MCP,
    All = UI | API
};
DECLARE_FLAGS(Availabilities, Availability)
DECLARE_OPERATORS_FOR_FLAGS(Availabilities)

//! NOTE Annotation for a command action (used in MCP)
enum class Annotation {
    None = 0,                 // Not readonly, not destructive, not idempotent, not open world (select next...)
    ReadOnly = 1 << 0,        // Does not change state (getters...)
    Destructive = 1 << 1,     // Can irreversibly change state (delete, cut...)
    Idempotent = 1 << 2,      // A repeated call produces no new effect (getters, selections...)
    OpenWorld = 1 << 3,       // Interacts with the outside world (network, file system, open, save...)
};
DECLARE_FLAGS(Annotations, Annotation)
DECLARE_OPERATORS_FOR_FLAGS(Annotations)

struct CommandInfo
{
    Command command;
    MnemonicString title;
    TranslatableString description;
    InputSchema inputSchema;
    OutputSchema outputSchema;
    Decoration decoration;
    Availabilities availabilities = Availability::All; // All temporarily, when we set the flag for all commands, it needs to be replaced with Availability::Disabled;
    Annotations annotations = Annotation::Destructive;  // Default to destructive

    CommandInfo() = default;
    CommandInfo(const Command& command, const MnemonicString& title, const TranslatableString& description,
                const InputSchema& inputSchema, const Decoration& decoration)
        : command(command), title(title), description(description), inputSchema(inputSchema), decoration(decoration) {}

    CommandInfo(const Command& command, const MnemonicString& title, const TranslatableString& description,
                const InputSchema& inputSchema, const Decoration& decoration, const Availabilities& availabilities)
        : command(command), title(title), description(description), inputSchema(inputSchema), decoration(decoration),
        availabilities(availabilities) {}

    CommandInfo(const Command& command, const MnemonicString& title, const TranslatableString& description,
                const InputSchema& inputSchema, const OutputSchema& outputSchema, const Decoration& decoration,
                const Availabilities& availabilities)
        : command(command), title(title), description(description), inputSchema(inputSchema), outputSchema(outputSchema),
        decoration(decoration), availabilities(availabilities) {}

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
    Command command;
    Params params;
};

struct Response
{
    CallId callId = 0;
    Ret ret;
    Params result;
};

inline CallId new_call_id()
{
    static CallId lastId = 0;
    ++lastId;
    return lastId;
}

inline Request make_request(const Command& command, const Params& params)
{
    Request request;
    request.callId = new_call_id();
    request.command = command;
    request.params = params;
    return request;
}

inline Response make_response(const Request& request, const Ret& ret)
{
    Response response;
    response.callId = request.callId;
    response.ret = ret;
    return response;
}

inline Response make_response(const Request& request, const Ret& ret, const Params& result)
{
    Response response;
    response.callId = request.callId;
    response.ret = ret;
    response.result = result;
    return response;
}
}

inline muse::logger::Stream& operator<<(muse::logger::Stream& s, const muse::rcommand::Params& p)
{
    s << "{";
    for (const auto& [key, value] : p) {
        s << key << ": " << value.toString() << ", ";
    }
    s << "}";
    return s;
}
