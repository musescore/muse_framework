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

#include <concepts>
#include <tuple>
#include <type_traits>
#include <utility>

#include "global/stringutils.h"

#include "commandtypes.h"

#include "log.h"

//! NOTE Convenience macro for declaring a static const inline auto property.
#define cprop_decl static const inline auto

/* Example of a typed command

struct SelectCommand {
    cprop_decl id = Command("command://somemodule/select");
    cprop_decl title = MnemonicString("action", "Select");
    cprop_decl description = TranslatableString("action", "Select items by target");
    cprop_decl availabilities = Availability::API;
    cprop_decl annotations = Annotation::None;
    cprop_decl input = FieldsScheme {
        Field{ "target", &SelectCommand::target, "Selection target" },
        Field{ "play-mode", &SelectCommand::playMode, "Play mode", Presence::Optional },
    };
};
*/

namespace muse::rcommand {
// concepts --------

// *INDENT-OFF* // Uncrustify doesn't understand `requires`
template<typename C>
concept TypedCommand = requires {
    { C::id } -> std::convertible_to<const rcommand::Command&>;
    { C::title } -> std::convertible_to<const MnemonicString&>;
    { C::description } -> std::convertible_to<const TranslatableString&>;
};

//! NOTE Same as `TypedCommand`, but accepts cv/ref qualified types as well,
//! for example the type of an expression or the result of `decltype()`.
template<typename T>
concept TypedCommandType = TypedCommand<std::remove_cvref_t<T> >;

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

// For bug detection
template<typename C>
concept HasAvailability = TypedCommand<C> && requires(const C& c) {
    C::availability;
};

template<typename C>
concept HasAnnotations = TypedCommand<C> && requires(const C& c) {
    { C::annotations } -> std::convertible_to<const Annotations&>;
};

//! NOTE An alias command delegates to another command and may fix some of its input values:
//!     `static const inline auto redirect = SomeCommand { ... };`
template<typename C>
concept HasRedirect = TypedCommand<C>
                      //! NOTE `decltype(C::redirect)` is cv-qualified (`const SomeCommand`),
                      //! that's why we compare through `TypedCommandType`
                      && TypedCommandType<decltype(C::redirect)>
                      //! NOTE it must be a static data member, not a field of an instance
                      && !std::is_member_pointer_v<decltype(&C::redirect)>;

// *INDENT-ON*

// required fields --------
namespace internal {
struct Construct;
}

//! A field every sender has to set.
//! The command stays an aggregate, so senders keep writing `C { .url = "…" }`,
//! but `C {}` or `C { .other = … }` no longer compile: the only public constructor takes a value.
//! Adding a Required<T> field to a command therefore breaks every sender that does not name it.
//! On the wire and in the schema it is a plain T.
template<typename T>
class Required
{
public:
    // *INDENT-OFF*
    template<typename U>
    requires (!std::is_same_v<std::remove_cvref_t<U>, Required> && std::is_constructible_v<T, U&&>)
    constexpr Required(U&& value)
        : m_value(std::forward<U>(value)) {}
    // *INDENT-ON*

    constexpr Required& operator=(const T& value)
    {
        m_value = value;
        return *this;
    }

    constexpr operator const T&() const
    {
        return m_value;
    }

    constexpr const T& value() const
    {
        return m_value;
    }

    friend constexpr bool operator==(const Required& lhs, const Required& rhs) { return lhs.m_value == rhs.m_value; }

    // *INDENT-OFF*
    //! A template, so that comparing with a value needing a conversion (a string literal) is not ambiguous
    //! with the reversed candidate C++20 synthesizes
    template<typename U>
    requires (!std::is_same_v<U, Required>)
    friend constexpr bool operator==(const Required& lhs, const U& rhs) { return lhs.m_value == rhs; }
    // *INDENT-ON*

private:
    //! Only the handler side builds a command before filling it from the request params
    constexpr Required() = default;
    friend struct internal::Construct;

    T m_value {};
};

//! The type a field carries on the wire: T for both `T` and `Required<T>` members
template<typename T>
struct FieldValue {
    using type = T;
};

template<typename T>
struct FieldValue<Required<T> > {
    using type = T;
};

template<typename T>
using field_value_t = typename FieldValue<T>::type;

template<typename T>
constexpr bool is_required_v = !std::is_same_v<T, field_value_t<T> >;

// data fields --------
enum class ConvertParamsPolicy {
    StrictCompliance,
    WarningUnknown,
    IgnoreUnknown,
};

struct ConvertParamsResult {
    Ret ret;
    std::vector<std::string> unknownParams; // Available in params, not in the schema
    std::vector<std::string> missingParams; // Required, but not available
    std::vector<std::string> invalidParams; // Available, but not converted to the target type

    operator bool() const
    {
        return ret.success();
    }

    void applyPolicy(ConvertParamsPolicy policy)
    {
        // missing/invalid params are always an error,
        // unknown params are an error only for StrictCompliance
        const bool ok = missingParams.empty()
                        && invalidParams.empty()
                        && (policy != ConvertParamsPolicy::StrictCompliance || unknownParams.empty());

        ret = ok ? make_ok() : muse::make_ret(Ret::Code::BadArgs, infoAsString());
    }

    void printResult(ConvertParamsPolicy policy) const
    {
        if (ret) {
            if (policy == ConvertParamsPolicy::WarningUnknown) {
                if (!unknownParams.empty()) {
                    LOGW() << "convert success, but unknown params: " << strings::join(unknownParams, ", ");
                    return;
                }
            }
            LOGD() << "convert success, info: " + infoAsString();
        } else {
            LOGE() << "convert failed, info: " + infoAsString();
        }
    }

    std::string infoAsString(const std::vector<std::string>& info) const
    {
        return info.empty() ? "none" : strings::join(info, ", ");
    }

    std::string infoAsString() const
    {
        std::stringstream ss;
        ss << "unknown params: " << infoAsString(unknownParams)
           << ", missing params: " << infoAsString(missingParams)
           << ", invalid params: " << infoAsString(invalidParams);
        return ss.str();
    }
};

template<typename S, typename T>
struct Field {
    std::string_view key;
    T S::* member = nullptr;
    TranslatableString label;
    TranslatableString description;
    std::optional<field_value_t<T> > min;
    std::optional<field_value_t<T> > max;
    Presence presence = Presence::Required;

    Field(std::string_view key, T S::* member, std::string_view description
          , Presence presence = Presence::Required)
        : key(key), member(member), description(TranslatableString::untranslatable(String::fromUtf8(description))),
        presence(presence) {}

    Field(std::string_view key, T S::* member, std::string_view description,
          const field_value_t<T>& min, const field_value_t<T>& max, Presence presence = Presence::Required)
        : key(key), member(member), description(TranslatableString::untranslatable(String::fromUtf8(description))),
        min(min), max(max), presence(presence) {}

    Field(std::string_view key, T S::* member, const TranslatableString& label, const TranslatableString& description
          , Presence presence = Presence::Required)
        : key(key), member(member), label(label), description(description), presence(presence) {}

    Field(std::string_view key, T S::* member, const TranslatableString& label, const TranslatableString& description,
          const field_value_t<T>& min, const field_value_t<T>& max, Presence presence = Presence::Required)
        : key(key), member(member), label(label), description(description), min(min), max(max), presence(presence) {}

    //! A Required<T> member is required whatever `presence` says
    bool isRequired() const { return is_required_v<T> || presence == Presence::Required; }
};

template<typename S, typename ... T>
using FieldsScheme = std::tuple<Field<S, T>...>;

template<typename E>
struct EnumContract;

namespace internal {
//! Builds a command on the handler side, before its fields are filled from the request params.
//! This is the only place allowed to leave a Required<T> field at its default value.
struct Construct {
    template<typename C>
    static C make()
    {
        return C {};
    }
};

// *INDENT-OFF* // Uncrustify doesn't understand `requires`
//! Number of non-static data members of an aggregate: the largest N for which `C { a1, …, aN }` is well-formed.
//! Counted downwards, because leaving a Required<T> member out of a partial initialization is ill-formed.
struct AnyValue {
    template<typename T>
    operator T() const;
};

template<typename C, std::size_t... I>
constexpr bool isAggregateInitializableWith(std::index_sequence<I...>)
{
    return requires { C { (static_cast<void>(I), AnyValue {})... }; };
}

template<typename C, std::size_t N = 32>
constexpr std::size_t memberCount()
{
    static_assert(std::is_aggregate_v<C>, "A typed command must be an aggregate");
    if constexpr (N == 0) {
        return 0;
    } else if constexpr (isAggregateInitializableWith<C>(std::make_index_sequence<N> {})) {
        return N;
    } else {
        return memberCount<C, N - 1>();
    }
}
// *INDENT-ON*

template<typename T>
const field_value_t<T>& valueOf(const T& v)
{
    if constexpr (is_required_v<T>) {
        return v.value();
    } else {
        return v;
    }
}

template<typename T>
DataType dataType()
{
    if constexpr (std::is_same<T, bool>::value) {
        return DataType::Bool;
    } else if constexpr (std::is_same<T, int>::value) {
        return DataType::Int;
    } else if constexpr (std::is_same<T, unsigned int>::value) {
        return DataType::Int;
    } else if constexpr (std::is_same<T, int64_t>::value) {
        return DataType::Int64;
    } else if constexpr (std::is_same<T, uint64_t>::value) {
        return DataType::Int64;
    } else if constexpr (std::is_same<T, float>::value) {
        return DataType::Float;
    } else if constexpr (std::is_same<T, double>::value) {
        return DataType::Double;
    } else if constexpr (std::is_same<T, std::string>::value) {
        return DataType::String;
    } else if constexpr (std::is_enum<T>::value) {
        return DataType::Enum;
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
    if constexpr (is_required_v<T>) {
        return toVal(val.value());
    } else if constexpr (std::is_enum<T>::value) {
        return Val(EnumContract<T>::str_val(val));
    } else if constexpr (std::is_same<T, unsigned int>::value) {
        return Val(static_cast<int>(val));
    } else if constexpr (std::is_same<T, uint64_t>::value) {
        return Val(static_cast<int64_t>(val));
    } else if constexpr (std::is_same<T, float>::value) {
        return Val(static_cast<double>(val));
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
        return EnumContract<T>::enum_val(v.toString());
    } else if constexpr (std::is_same<T, unsigned int>::value) {
        auto opt = v.toType<int>();
        if (opt) {
            return static_cast<unsigned int>(*opt);
        }
        return std::nullopt;
    } else if constexpr (std::is_same<T, uint64_t>::value) {
        auto opt = v.toType<int64_t>();
        if (opt) {
            return static_cast<uint64_t>(*opt);
        }
        return std::nullopt;
    } else if constexpr (std::is_same<T, float>::value) {
        auto opt = v.toType<double>();
        if (opt) {
            return static_cast<float>(*opt);
        }
        return std::nullopt;
    } else {
        return v.toType<T>();
    }
}

template<typename S, typename T>
bool fromVal(S& o, T S::* member, const Val& v)
{
    std::optional<field_value_t<T> > value = fromVal<field_value_t<T> >(v);
    if (value) {
        o.*member = *value;
        return true;
    }

    return false;
}

template<typename S, typename T>
bool fromVal(S& o, const Field<S, T>& f, const Val& v)
{
    return fromVal(o, f.member, v);
}

template<typename T>
bool isWithinBounds(const T& value, const std::optional<T>& min, const std::optional<T>& max)
{
    //! NOTE The comparisons are negated on purpose.
    //! For NaN both `value < min` and `value > max` are false, so checking only
    //! "below the minimum" / "above the maximum" would let NaN through.
    //! Requiring the value to be inside the bounds rejects it, because `NaN >= min` is false.
    if (min && !(value >= *min)) {
        return false;
    }
    if (max && !(value <= *max)) {
        return false;
    }
    return true;
}

template<typename S, typename T>
void convert(S& o, const Field<S, T>& f, const Params& params, ConvertParamsResult& res)
{
    if (params.find(f.key) == params.end()) {
        if (f.isRequired()) {
            res.missingParams.push_back(std::string(f.key));
        }
        return;
    }

    const Val& v = params.at(f.key);

    if (v.isNull()) {
        if (f.isRequired()) {
            res.invalidParams.push_back(std::string(f.key));
        }
        return;
    }

    bool ok = fromVal(o, f.member, v);
    if (!ok) {
        res.invalidParams.push_back(std::string(f.key));
        return;
    }

    if (!isWithinBounds(valueOf(o.*f.member), f.min, f.max)) {
        res.invalidParams.push_back(std::string(f.key));
    }
}

template<typename F>
constexpr auto keysOf(const F& fields)
{
    std::array<std::string_view, std::tuple_size_v<F> > keys{};
    std::apply([&keys](const auto&... f) {
        std::size_t i = 0;
        ((keys[i++] = f.key), ...);
    }, fields);
    return keys;
}

template<typename F>
std::vector<std::string> unknownKeys(const Params& params, const F& fields)
{
    const auto keys = keysOf(fields);
    std::vector<std::string> unknown;
    for (const auto& [key, value] : params) {
        if (std::find(keys.begin(), keys.end(), std::string_view(key)) == keys.end()) {
            unknown.push_back(key);
        }
    }
    return unknown;
}

template<typename S, typename T>
Arg makeArg(const Field<S, T>& f, int order)
{
    using V = field_value_t<T>;
    Arg arg(dataType<V>(), f.label, f.description, f.isRequired() ? Presence::Required : Presence::Optional);
    arg.uiHints = UiHints { order };

    if constexpr (std::is_enum<V>::value) {
        arg.allowedValues = toValList(EnumContract<V>::str_vals());
    }

    if (f.min) {
        arg.min = toVal(*f.min);
    }
    if (f.max) {
        arg.max = toVal(*f.max);
    }
    return arg;
}

template<typename O, typename F>
Params toParams(const O& o, const F& fields)
{
    Params params;
    std::apply([&](const auto&... f) {
        (params.emplace(f.key, internal::toVal(o, f.member)), ...);
    }, fields);
    return params;
}

template<typename O, typename F>
ConvertParamsResult fromParams(const Params& params, O& o, const F& fields, ConvertParamsPolicy policy)
{
    ConvertParamsResult res;
    std::apply([&](const auto&... f) {
        ((internal::convert(o, f, params, res)), ...);
    }, fields);

    res.unknownParams = internal::unknownKeys(params, fields);

    res.applyPolicy(policy);

    res.printResult(policy);

    return res;
}
}     // namespace internal

// dispatch (params) --------
template<HasInput C>
Params toInputParams(const C& o)
{
    return internal::toParams(o, C::input);
}

template<HasInput C>
ConvertParamsResult fromInputParams(const Params& params, C& o, ConvertParamsPolicy policy = ConvertParamsPolicy::StrictCompliance)
{
    return internal::fromParams(params, o, C::input, policy);
}

template<HasOutput C>
Params toOutputParams(const typename C::Output& o)
{
    return internal::toParams(o, C::output);
}

template<HasOutput C>
ConvertParamsResult fromOutputParams(const Params& params, typename C::Output& o,
                                     ConvertParamsPolicy policy = ConvertParamsPolicy::StrictCompliance)
{
    return internal::fromParams(params, o, C::output, policy);
}

// command info --------

//! A member left out of `input` (or `output`) would silently never be sent
template<typename S, typename F>
constexpr bool fieldsCoverAllMembers = std::tuple_size_v<std::remove_cv_t<F> > == internal::memberCount<S>();

template<HasInput C>
InputSchema inputSchema()
{
    static_assert(fieldsCoverAllMembers<C, decltype(C::input)>, "Every data member of the command must be declared in `input`");

    InputSchema schema;
    int order = 0;
    std::apply([&](const auto&... f) {
        (schema.args.emplace(f.key, internal::makeArg(f, ++order)), ...);
    }, C::input);
    return schema;
}

template<HasOutput C>
OutputSchema outputSchema()
{
    static_assert(fieldsCoverAllMembers<typename C::Output, decltype(C::output)>,
                  "Every data member of the command's Output must be declared in `output`");

    OutputSchema schema;
    int order = 0;
    std::apply([&](const auto&... f) {
        (schema.args.emplace(f.key, internal::makeArg(f, ++order)), ...);
    }, C::output);
    return schema;
}

template<TypedCommand C>
CommandInfo makeCommandInfo()
{
    CommandInfo info;
    info.command = C::id;
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

    static_assert(!HasAvailability<C>, "`availability` is not correct, use `availabilities` instead");
    if constexpr (HasAvailabilities<C>) {
        info.availabilities = C::availabilities;
    }

    if constexpr (HasAnnotations<C>) {
        info.annotations = C::annotations;
    }

    if constexpr (HasRedirect<C>) {
        Params params;
        using Redirect = std::remove_cvref_t<decltype(C::redirect)>;
        if constexpr (HasInput<Redirect>) {
            params = toInputParams(C::redirect);
        }
        info.redirect = std::make_pair(C::redirect.id, params);
    }
    return info;
}
}
