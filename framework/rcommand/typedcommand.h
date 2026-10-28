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

#include <tuple>
#include "commandtypes.h"

namespace muse::rcommand {
// concepts --------

// *INDENT-OFF* // Uncrustify doesn't understand `requires`
template<typename C>
concept TypedCommand = requires {
    { C::command } -> std::convertible_to<const rcommand::Command&>;
    { C::title } -> std::convertible_to<const MnemonicString&>;
    { C::description } -> std::convertible_to<const TranslatableString&>;
};

template<typename C>
concept HasInput = TypedCommand<C> && requires(const C& c) {
    c.input;
};

template<typename C>
concept HasOutput = TypedCommand<C> && requires(const C& c) {
    typename C::Output;
    c.output;
};

template<typename C>
concept HasDecoration = TypedCommand<C> && requires(const C& c) {
    { C::decoration } -> std::convertible_to<const Decoration&>;
};

template<typename C>
concept HasAvailabilities = TypedCommand<C> && requires(const C& c) {
    { C::availabilities } -> std::convertible_to<const Availabilities&>;
};

// *INDENT-ON*

// data fields --------

enum class FieldType {
    Required,
    Optional,
};

template<typename S, typename T>
struct Field {
    std::string_view key;
    T S::* member = nullptr;
    std::string_view description;
    std::optional<T> min;
    std::optional<T> max;
    FieldType type = FieldType::Required;

    constexpr Field(std::string_view key, T S::* member, std::string_view description
                    , FieldType type = FieldType::Required)
        : key(key), member(member), description(description), type(type) {}

    constexpr Field(std::string_view key, T S::* member, std::string_view description,
                    const T& min, const T& max, FieldType type = FieldType::Required)
        : key(key), member(member), description(description), min(min), max(max), type(type) {}
};

// enum contract
template<typename E>
struct Contract;

namespace internal {
template<typename T>
DataType dataType()
{
    if constexpr (std::is_same<T, bool>::value) {
        return DataType::Bool;
    } else if constexpr (std::is_same<T, int>::value) {
        return DataType::Int;
    } else if constexpr (std::is_same<T, int64_t>::value) {
        return DataType::Int64;
    } else if constexpr (std::is_same<T, float>::value) {
        return DataType::Float;
    } else if constexpr (std::is_same<T, double>::value) {
        return DataType::Double;
    } else if constexpr (std::is_same<T, std::string>::value) {
        return DataType::String;
    } else if constexpr (std::is_enum<T>::value) {  // enum as string!!
        return DataType::String;
    } else if constexpr (std::is_same<T, std::vector<T> >::value) {
        return DataType::List;
    } else if constexpr (std::is_same<T, std::map<std::string, T> >::value) {
        return DataType::Map;
    } else {
        static_assert(std::is_same<T, void>::value, "Unsupported type");
        return DataType::Undefined;
    }
}

template<typename T>
Val toVal(const T& val)
{
    if constexpr (std::is_enum<T>::value) {
        return Val(Contract<T>::str_val(val));
    } else {
        return Val::fromType(val);
    }
}

template<typename S, typename T>
Val toVal(const S& o, const T S::* member)
{
    return toVal(o.*member);
}

template<typename T>
std::optional<T> fromVal(const Val& v)
{
    if constexpr (std::is_enum<T>::value) {
        return Contract<T>::enum_val(v.toString());
    } else {
        return v.toType<T>();
    }
}

template<typename S, typename T>
bool fromVal(S& o, T S::* member, const Val& v)
{
    std::optional<T> value = fromVal<T>(v);
    if (value) {
        o.*member = *value;
        return true;
    }

    return false;
}

template<typename S, typename T>
bool fromVal(S& o, const Field<S, T>& f, const Val& v)
{
    if (v.isNull()) {
        return (f.type == FieldType::Required) ? false : true;
    } else {
        return fromVal(o, f.member, v);
    }
}

template<typename S, typename T>
Arg makeArg(const Field<S, T>& f)
{
    Arg arg(dataType<T>(), String::fromUtf8(f.description));
    if (f.min) {
        arg.min = toVal(*f.min);
    }
    if (f.max) {
        arg.max = toVal(*f.max);
    }
    return arg;
}
} // namespace internal

// command info --------

template<HasInput C>
InputSchema inputSchema()
{
    std::map<std::string, Arg> args;
    std::apply([&](const auto&... f) {
        (args.emplace(f.key, internal::makeArg(f)), ...);
    }, C::input);
    return InputSchema(std::move(args));
}

template<HasOutput C>
OutputSchema outputSchema()
{
    std::map<std::string, Arg> args;
    std::apply([&](const auto&... f) {
        (args.emplace(f.key, internal::makeArg(f)), ...);
    }, C::output);
    return OutputSchema(std::move(args));
}

template<TypedCommand C>
CommandInfo makeCommandInfo()
{
    CommandInfo info;
    info.command = C::command;
    info.title = C::title;
    info.description = C::description;
    if constexpr (HasInput<C>) {
        info.inputSchema = inputSchema<C>();
    }
    if constexpr (HasOutput<C>) {
        info.outputSchema = outputSchema<C>();
    }
    if constexpr (HasDecoration<C>) {
        info.decoration = C::decoration;
    }
    if constexpr (HasAvailabilities<C>) {
        info.availabilities = C::availabilities;
    }
    return info;
}

// dispatch (params) --------
template<HasInput C>
Params toInputParams(const C& o)
{
    Params params;
    std::apply([&](const auto&... f) {
        (params.emplace(f.key, internal::toVal(o, f.member)), ...);
    }, C::input);
    return params;
}

template<HasInput C>
bool fromInputParams(const Params& params, C& o)
{
    bool ok = true;
    std::apply([&](const auto&... f) {
        ((ok = ok && internal::fromVal(o, f, params.at(f.key))), ...);
    }, C::input);
    return ok;
}

template<HasOutput C>
Params toOutputParams(const typename C::Output& o)
{
    Params params;
    std::apply([&](const auto&... f) {
        (params.emplace(f.key, internal::toVal(o, f.member)), ...);
    }, C::output);
    return params;
}

template<HasOutput C>
bool fromOutputParams(const Params& params, typename C::Output& o)
{
    bool ok = true;
    std::apply([&](const auto&... f) {
        ((ok = ok && internal::fromVal(o, f, params.at(f.key))), ...);
    }, C::output);
    return ok;
}
}
