/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2022 MuseScore Limited and others
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

#include <type_traits>

namespace muse {
template<typename T>
struct required_t
{
    using value_type = T;

    T value {};

    explicit required_t() = default;
    required_t(T v)
        : value(v) {}

    operator T() const {
        return value;
    }

    T& operator=(T v) { value = v; return value; }

    T& operator*() { return value; }

    T& operator->() { return value; }

    bool operator==(const required_t<T>& other) const { return value == other.value; }
    bool operator!=(const required_t<T>& other) const { return value != other.value; }
    bool operator<(const required_t<T>& other) const { return value < other.value; }
    bool operator>(const required_t<T>& other) const { return value > other.value; }
    bool operator<=(const required_t<T>& other) const { return value <= other.value; }
    bool operator>=(const required_t<T>& other) const { return value >= other.value; }

    bool operator==(const T& other) const { return value == other; }
    bool operator!=(const T& other) const { return value != other; }
    bool operator<(const T& other) const { return value < other; }
    bool operator>(const T& other) const { return value > other; }
    bool operator<=(const T& other) const { return value <= other; }
    bool operator>=(const T& other) const { return value >= other; }
};

template<typename T> struct is_required_t : std::false_type {};
template<typename T> struct is_required_t<required_t<T> > : std::true_type {};
}
