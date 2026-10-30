/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
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
#ifndef MUSE_GLOBAL_VAL_H
#define MUSE_GLOBAL_VAL_H

#include <string>
#include <string_view>
#include <variant>
#include <type_traits>
#include <vector>
#include <map>
#include <optional>

#include "number.h"
#include "io/path.h"

#ifndef NO_QT_SUPPORT
#include <QString>
#include <QColor>
#include <QVariant>
#endif

namespace muse {
class Val;
using ValList = std::vector<Val>;
using ValMap = std::map<std::string, Val>;
class Val
{
public:
    enum class Type {
        Undefined = 0,
        Bool,
        Int,
        Int64,
        Double,
        String,
        List,
        Map,
        Color
    };

    Val() = default;
    explicit Val(bool val);
    explicit Val(int val);
    explicit Val(int64_t val);
    explicit Val(double val);
    template<typename T>
    explicit Val(number_t<T> val)
        : Val{val.raw()} {}
    explicit Val(const std::string& str);
    explicit Val(const char* str);
    explicit Val(const io::path_t& path);
    explicit Val(const ValList& list);
    explicit Val(const ValMap& map);

    template<class E, typename = std::enable_if_t<std::is_enum_v<E> > >
    explicit Val(E val)
        : Val{static_cast<std::underlying_type_t<E> >(val)}
    {
    }

    void setType(Type t);
    Type type() const;

    bool isNull() const;
    bool toBool() const;
    int toInt() const;
    int64_t toInt64() const;
    double toDouble() const;
    float toFloat() const;
    std::string toString() const;
    io::path_t toPath() const;
    ValList toList() const;
    ValMap toMap() const;

    template<class E, typename = std::enable_if_t<std::is_enum_v<E> > >
    E toEnum() const
    {
        return static_cast<E>(toInt());
    }

    template<typename T>
    std::optional<T> toType() const
    {
        if constexpr (std::is_same<T, bool>::value) {
            return doToBool();
        } else if constexpr (std::is_same<T, int>::value) {
            return doToInt();
        } else if constexpr (std::is_same<T, int64_t>::value) {
            return doToInt64();
        } else if constexpr (std::is_same<T, double>::value) {
            return doToDouble();
        } else if constexpr (is_number<T>()) {
            auto value = toType<typename T::value_type>();
            if (!value) {
                return std::nullopt;
            }
            return T(*value);
        } else if constexpr (std::is_same<T, std::string>::value) {
            return doToString();
        } else if constexpr (std::is_same<T, io::path_t>::value) {
            return doToPath();
        } else if constexpr (std::is_same<T, ValList>::value) {
            return doToList();
        } else if constexpr (std::is_same<T, ValMap>::value) {
            return doToMap();
        } else if constexpr (is_vector<T>::value) {
            auto list = doToList();
            if (!list) {
                return std::nullopt;
            }
            T result;
            for (const auto& item : *list) {
                auto value = item.toType<typename T::value_type>();
                if (!value) {
                    return std::nullopt;
                }
                result.push_back(*value);
            }
            return result;
        } else if constexpr (is_string_map<T>()) {
            auto map = doToMap();
            if (!map) {
                return std::nullopt;
            }
            T result;
            for (const auto& item : *map) {
                auto value = item.second.toType<typename T::mapped_type>();
                if (!value) {
                    return std::nullopt;
                }
                result[item.first] = *value;
            }
            return result;
        } else {
            static_assert(std::is_same<T, void>::value, "Unsupported type");
            return std::nullopt;
        }
    }

    template<typename T>
    static Val fromType(const T& value)
    {
        if constexpr (std::is_same<T, bool>::value) {
            return Val(value);
        } else if constexpr (std::is_same<T, int>::value) {
            return Val(value);
        } else if constexpr (std::is_same<T, int64_t>::value) {
            return Val(value);
        } else if constexpr (std::is_same<T, double>::value) {
            return Val(value);
        } else if constexpr (is_number<T>()) {
            return Val(value);
        } else if constexpr (std::is_same<T, std::string>::value) {
            return Val(value);
        } else if constexpr (std::is_same<T, io::path_t>::value) {
            return Val(value);
        } else if constexpr (std::is_same<T, ValList>::value) {
            return Val(value);
        } else if constexpr (std::is_same<T, ValMap>::value) {
            return Val(value);
        } else if constexpr (is_vector<T>::value) {
            ValList list;
            for (const auto& item : value) {
                list.push_back(fromType(item));
            }
            return Val(list);
        } else if constexpr (is_string_map<T>()) {
            ValMap map;
            for (const auto& item : value) {
                map[item.first] = fromType(item.second);
            }
            return Val(map);
        } else {
            static_assert(std::is_same<T, void>::value, "Unsupported type");
            return Val();
        }
    }

    bool operator ==(const Val& v) const;
    bool operator <(const Val& v) const;

#ifndef NO_QT_SUPPORT
    explicit Val(const QString& str);
    explicit Val(const QColor& color);

    QString toQString() const;
    QColor toQColor() const;

    QVariant toQVariant() const;
    static Val fromQVariant(const QVariant& var);
#endif

private:

    template<typename T> struct is_number : std::false_type {};
    template<typename T> struct is_number<number_t<T> > : std::true_type {};

    template<typename T> struct is_vector : std::false_type {};
    template<typename U, typename A> struct is_vector<std::vector<U, A> > : std::true_type {};

    template<typename T> struct is_string_map : std::false_type {};
    template<typename U, typename C, typename A> struct is_string_map<std::map<std::string, U, C, A> > : std::true_type {};

    Type valueType() const;

    std::optional<bool> doToBool() const;
    std::optional<int> doToInt() const;
    std::optional<int64_t> doToInt64() const;
    std::optional<double> doToDouble() const;
    std::optional<std::string> doToString() const;
    std::optional<io::path_t> doToPath() const;
    std::optional<ValList> doToList() const;
    std::optional<ValMap> doToMap() const;

    //! NOTE Should be sync with valueType method
    using Value = std::variant<std::monostate, bool, int, int64_t, double, std::string, ValList, ValMap>;
    Value m_val;
    Type m_type = Type::Undefined;
};

//! NOTE Helper class for the serialization/deserialization of lists and maps
class ValJson
{
public:
    static bool toString(const ValList& list, std::string& str);
    static bool toString(const ValMap& map, std::string& str);
    static bool fromString(const std::string& str, ValList& list);
    static bool fromString(const std::string& str, ValMap& map);

private:
    static std::string_view trim(std::string_view str);
    static bool isIntegerToken(std::string_view str);
    static bool isDoubleToken(std::string_view str);
    static bool splitTopLevel(std::string_view str, std::vector<std::string_view>& out);
    static size_t topLevelColon(std::string_view str);
    static std::optional<Val> parseValue(std::string_view token);
    static bool parseList(std::string_view str, ValList& list);
    static bool parseMap(std::string_view str, ValMap& map);
};
}

#endif // MUSE_GLOBAL_VAL_H
