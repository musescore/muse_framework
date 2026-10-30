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

#include "rcommand/commandtypes.h"

#include "log.h"

namespace muse::rcommand {
template<typename E>
struct Contract;

template<typename T>
concept Enumeration = std::is_enum_v<std::remove_cvref_t<T> >;

template<typename E>
struct EnumEntry {
    std::string_view sval;
    E eval;
    std::string_view description;
};

template<typename E, std::size_t N>
using Table = std::array<EnumEntry<E>, N>;

template<typename E, std::size_t N>
void accepts_table(const Table<E, N>&);

template<typename T, typename E>
concept EnumTable = requires(const std::remove_cvref_t<T>& table) {
    accepts_table<E>(table);
};

// *INDENT-OFF* // Uncrustify doesn't understand `requires`
template<typename C>
concept EnumContract = requires {
    { C::key } -> std::convertible_to<const std::string_view&>;
    { C::def } -> Enumeration;
    { C::table } -> EnumTable<std::remove_cvref_t<decltype(C::def)>>;
};
// *INDENT-ON*

template<EnumContract C>
struct BaseContract {
    static std::string str_val(decltype(C::def) eval)
    {
        for (const auto& row : C::table) {
            if (row.eval == eval) {
                return std::string(row.sval);
            }
        }
        UNREACHABLE;
        return std::string();
    }

    static std::optional<decltype(C::def)> enum_val(const std::string& sval)
    {
        for (const auto& row : C::table) {
            if (row.sval == sval) {
                return row.eval;
            }
        }
        return std::nullopt;
    }

    static std::pair<std::string, muse::Val> param(decltype(C::def) eval)
    {
        return { std::string(C::key), muse::Val(str_val(eval)) };
    }

    static decltype(C::def) get(const muse::rcommand::Params& params)
    {
        auto sval = params.at(C::key).toString();
        auto value = enum_val(sval);
        return value ? *value : C::def;
    }

    static void set(muse::rcommand::Params& params, decltype(C::def) eval)
    {
        params.set(param(eval));
    }
};

#define DECLARE_CONTRACT(E, C) template<> struct Contract<E> : BaseContract<C> {};
}
