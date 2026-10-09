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

#include <cmath>
#include <concepts>
#include <cstdint>
#include <limits>
#include <locale>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

#include "global/types/string.h"
#include "global/types/val.h"

#include "commandtypes.h"

//! Typed commands
//!
//! One struct per command is the single source of truth for its id, its texts
//! and its parameters. The input schema, the Params (de)serialization and the
//! handler signature are derived from it, so they cannot drift apart.
//!
//!   struct ChangePlayRegion {
//!       static inline const Command id { "command://playback/play-region/change" };
//!       static inline const TranslatableString title = TranslatableString("action", "Change play region");
//!       static inline const TranslatableString description = TranslatableString("action_description", "Change play region");
//!
//!       double start = 0.0;
//!       double end = 0.0;
//!
//!       static constexpr auto fields()
//!       {
//!           return std::tuple {
//!               Field { "start", &ChangePlayRegion::start, u"Region start in seconds" },
//!               Field { "end",   &ChangePlayRegion::end,   u"Region end in seconds" },
//!           };
//!       }
//!   };
//!
//!   register:  makeCommandInfo<ChangePlayRegion>()
//!   send:      dispatcher()->dispatch(ChangePlayRegion { 1.0, 5.0 });
//!   menu item: makeMenuItem(ChangePlayRegion { 1.0, 5.0 })
//!   receive:   dispatcher()->onRequest<ChangePlayRegion>(this, [this](const ChangePlayRegion& c) { ... });
//!
//! The untyped API keeps working for the same command: Params that arrive
//! from a shortcut, the API, MCP or a CommandQuery are validated against
//! the struct before the typed handler is called.

namespace muse::rcommand {
// =========================================================================
// Field list
// =========================================================================

//! One parameter of a typed command.
//! min / max are optional and only go to the schema (Arg::min / Arg::max).
template<typename S, typename T>
struct Field {
    const char* key = nullptr;
    T S::* member = nullptr;
    const char16_t* description = u"";
    double min = std::numeric_limits<double>::quiet_NaN();
    double max = std::numeric_limits<double>::quiet_NaN();
};

template<typename S, typename T>
Field(const char*, T S::*, const char16_t*)->Field<S, T>;

template<typename S, typename T>
Field(const char*, T S::*, const char16_t*, double, double)->Field<S, T>;

//! A typed command is a struct with a static `id` (its Command), a static `title`
//! and `description`, and a static `fields()` returning a std::tuple of Field.
// *INDENT-OFF* // Uncrustify doesn't understand `requires`
template<typename C>
concept TypedCommand = requires {
    { C::id } -> std::convertible_to<const Command&>;
    { C::title } -> std::convertible_to<const TranslatableString&>;
    { C::description } -> std::convertible_to<const TranslatableString&>;
    C::fields();
};

//! A command may also carry a static `decoration`; without it the default is used
template<typename C>
concept HasDecoration = requires {
    { C::decoration } -> std::convertible_to<const Decoration&>;
};

//! A command may also carry its static `availabilities` (see ADR 00107)
template<typename C>
concept HasAvailabilities = requires {
    { C::availabilities } -> std::convertible_to<const Availabilities&>;
};
// *INDENT-ON*

// =========================================================================
// ParamTraits: how one C++ type maps to DataType and to/from Val.
// Specialize it for domain types (enums, ids, secs_t...) to use them as fields.
// A field of a type without ParamTraits does not compile.
//
// Numbers and booleans are also accepted as strings, because the values
// of a CommandQuery come from a URI string.
// =========================================================================

template<typename T, typename = void>
struct ParamTraits;

namespace detail {
//! Locale-independent parse of the whole string
template<typename N>
bool parseNumber(const std::string& s, N& out)
{
    std::istringstream ss(s);
    ss.imbue(std::locale::classic());
    ss >> out;
    return !ss.fail() && ss.eof();
}
}

template<>
struct ParamTraits<bool> {
    static constexpr DataType dataType = DataType::Boolean;

    static Val toVal(bool v) { return Val(v); }

    static bool fromVal(const Val& v, bool& out)
    {
        switch (v.type()) {
        case Val::Type::Bool:
            out = v.toBool();
            return true;
        case Val::Type::String: {
            const std::string s = v.toString();
            if (s == "true" || s == "1") {
                out = true;
                return true;
            }
            if (s == "false" || s == "0") {
                out = false;
                return true;
            }
            return false;
        }
        default:
            return false;
        }
    }
};

template<typename T>
struct ParamTraits<T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool> > > {
    static constexpr DataType dataType = DataType::Integer;

    static Val toVal(T v)
    {
        if constexpr (sizeof(T) < sizeof(int) || (std::is_signed_v<T> && sizeof(T) == sizeof(int))) {
            return Val(static_cast<int>(v));
        } else {
            return Val(static_cast<int64_t>(v));
        }
    }

    static bool fromVal(const Val& v, T& out)
    {
        switch (v.type()) {
        case Val::Type::Int:
            out = static_cast<T>(v.toInt());
            return true;
        case Val::Type::Int64:
            out = static_cast<T>(v.toInt64());
            return true;
        case Val::Type::String: {
            int64_t n = 0;
            if (!detail::parseNumber(v.toString(), n)) {
                return false;
            }
            out = static_cast<T>(n);
            return true;
        }
        default:
            return false;
        }
    }
};

template<typename T>
struct ParamTraits<T, std::enable_if_t<std::is_floating_point_v<T> > > {
    static constexpr DataType dataType = DataType::Float;

    static Val toVal(T v) { return Val(static_cast<double>(v)); }

    //! Integers are accepted too: callers send 1 where 1.0 is meant.
    static bool fromVal(const Val& v, T& out)
    {
        switch (v.type()) {
        case Val::Type::Double:
            out = static_cast<T>(v.toDouble());
            return true;
        case Val::Type::Int:
            out = static_cast<T>(v.toInt());
            return true;
        case Val::Type::Int64:
            out = static_cast<T>(v.toInt64());
            return true;
        case Val::Type::String: {
            double d = 0.0;
            if (!detail::parseNumber(v.toString(), d)) {
                return false;
            }
            out = static_cast<T>(d);
            return true;
        }
        default:
            return false;
        }
    }
};

template<>
struct ParamTraits<std::string> {
    static constexpr DataType dataType = DataType::String;

    static Val toVal(const std::string& v) { return Val(v); }

    static bool fromVal(const Val& v, std::string& out)
    {
        if (v.type() != Val::Type::String) {
            return false;
        }
        out = v.toString();
        return true;
    }
};

template<>
struct ParamTraits<String> {
    static constexpr DataType dataType = DataType::String;

    static Val toVal(const String& v) { return Val(v.toStdString()); }

    static bool fromVal(const Val& v, String& out)
    {
        if (v.type() != Val::Type::String) {
            return false;
        }
        out = String::fromStdString(v.toString());
        return true;
    }
};

// =========================================================================
// Generated from the field list: inputSchema, toParams, fromParams
// =========================================================================

namespace detail {
//! A std::optional<T> field may be absent from Params.
//! Arg has no "required" flag, so the schema does not show the difference.
template<typename T>
struct Optional : std::false_type {
    using type = T;
};

template<typename T>
struct Optional<std::optional<T> > : std::true_type {
    using type = T;
};

template<typename V>
Val limitToVal(double limit)
{
    if constexpr (std::is_constructible_v<V, double>) {
        if (!std::isnan(limit)) {
            return ParamTraits<V>::toVal(static_cast<V>(limit));
        }
    }
    return Val();
}

template<typename S, typename T>
Arg makeArg(const Field<S, T>& f)
{
    using V = typename Optional<T>::type;
    return Arg(ParamTraits<V>::dataType, String(f.description), limitToVal<V>(f.min), limitToVal<V>(f.max));
}

template<typename S, typename T>
void writeField(Params& out, const S& obj, const Field<S, T>& f)
{
    using V = typename Optional<T>::type;
    const T& value = obj.*f.member;

    if constexpr (Optional<T>::value) {
        if (value.has_value()) {
            out[f.key] = ParamTraits<V>::toVal(*value);
        }
    } else {
        out[f.key] = ParamTraits<V>::toVal(value);
    }
}

template<typename S, typename T>
bool readField(const Params& in, S& obj, const Field<S, T>& f, size_t& matched, std::string& err)
{
    using V = typename Optional<T>::type;

    const auto it = in.find(f.key);
    if (it == in.end()) {
        if (Optional<T>::value) {
            return true;
        }
        err = std::string("missing parameter '") + f.key + "'";
        return false;
    }
    ++matched;

    V value {};
    if (!ParamTraits<V>::fromVal(it->second, value)) {
        err = std::string("parameter '") + f.key + "' has the wrong type";
        return false;
    }

    obj.*f.member = std::move(value);
    return true;
}
}

//! The schema for CommandInfo, generated instead of hand-written
template<TypedCommand C>
InputSchema inputSchema()
{
    std::map<std::string, Arg> args;
    std::apply([&](const auto&... f) {
        (args.emplace(f.key, detail::makeArg(f)), ...);
    }, C::fields());
    return InputSchema(std::move(args));
}

//! The CommandInfo of a typed command, entirely from the struct: `id`, `title`,
//! `description`, the schema from `fields()`, and `decoration` / `availabilities` when declared
template<TypedCommand C>
CommandInfo makeCommandInfo()
{
    CommandInfo info { C::id, C::title, C::description, inputSchema<C>() };
    if constexpr (HasDecoration<C>) {
        info.decoration = C::decoration;
    }
    if constexpr (HasAvailabilities<C>) {
        info.availabilities = C::availabilities;
    }
    return info;
}

template<TypedCommand C>
Params toParams(const C& c)
{
    Params out;
    std::apply([&](const auto&... f) {
        (detail::writeField(out, c, f), ...);
    }, C::fields());
    return out;
}

//! The single runtime validation point for Params that arrive untyped.
//! Rejects missing keys, wrong types and keys the command does not declare.
template<TypedCommand C>
bool fromParams(const Params& in, C& out, std::string& err)
{
    //! NOTE: a comma fold rather than `return (... && ...)`: the code style
    //! checker strips the parentheses a fold expression requires in a return.
    size_t matched = 0;
    bool ok = true;
    std::apply([&](const auto&... f) {
        ((ok = ok && detail::readField(in, out, f, matched, err)), ...);
    }, C::fields());

    if (!ok) {
        return false;
    }

    if (matched != in.size()) {
        err = "unknown parameter(s) passed";
        return false;
    }
    return true;
}
}
