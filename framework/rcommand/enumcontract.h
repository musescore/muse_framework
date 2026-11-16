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

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

#include "log.h"

namespace muse::rcommand {
template<typename E>
struct EnumContract;

template<typename T>
concept Enumeration = std::is_enum_v<std::remove_cvref_t<T> >;

template<Enumeration E>
struct EnumEntry {
    std::string_view sval;
    E eval;
    std::string_view description;
};

template<Enumeration E, std::size_t N>
using EnumTable = std::array<EnumEntry<E>, N>;

namespace internal {
//! NOTE `EnumTable` is an alias template, so it can't be matched as is.
//! This trait recognizes `std::array<EnumEntry<E>, N>` and recovers `E` and `N`.
template<typename T>
struct EnumTableTraits {
    static constexpr bool valid = false;
};

template<Enumeration E, std::size_t N>
struct EnumTableTraits<std::array<EnumEntry<E>, N> > {
    static constexpr bool valid = true;
    using Enum = E;
};
}

template<typename T>
concept IsEnumTable = internal::EnumTableTraits<std::remove_cvref_t<T> >::valid;

//! NOTE The enum type of an `EnumTable`, e.g. `Foo` for `EnumTable<Foo, 3>`.
template<typename T>
using EnumTableEnum = typename internal::EnumTableTraits<std::remove_cvref_t<T> >::Enum;

// *INDENT-OFF* // Uncrustify doesn't understand `requires`
template<typename C>
concept IsEnumContract = requires {
    { C::table } -> IsEnumTable<>;
};
// *INDENT-ON*

template<IsEnumContract C>
struct BaseEnumContract {
    using Enum = EnumTableEnum<decltype(C::table)>;

    static std::vector<std::string> str_vals()
    {
        std::vector<std::string> result;
        for (const auto& row : C::table) {
            result.push_back(std::string(row.sval));
        }
        return result;
    }

    static std::string str_val(Enum eval)
    {
        for (const auto& row : C::table) {
            if (row.eval == eval) {
                return std::string(row.sval);
            }
        }
        UNREACHABLE;
        return std::string();
    }

    static std::optional<Enum> enum_val(const std::string& sval)
    {
        for (const auto& row : C::table) {
            if (row.sval == sval) {
                return row.eval;
            }
        }
        return std::nullopt;
    }
};

//! NOTE The enum type is deduced from the contract's `table`.
#define DECLARE_ENUM_CONTRACT(C) template<> struct EnumContract<EnumTableEnum<decltype(C::table)> > : BaseEnumContract<C> {};
}
