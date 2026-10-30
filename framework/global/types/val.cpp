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
#include "val.h"

#include <cctype>
#include <limits>
#include <sstream>
#include <iomanip>
#include <string_view>

#ifndef NO_QT_SUPPORT
#include <QMetaType>
#endif

#include "log.h"

using namespace muse;

static const std::string VAL_TRUE("true");
static const std::string VAL_FALSE("false");

Val::Val(bool val)
    : m_val(val), m_type(Type::Bool) {}

Val::Val(int val)
    : m_val(val), m_type(Type::Int) {}

Val::Val(int64_t val)
    : m_val(val), m_type(Type::Int64) {}

Val::Val(double val)
    : m_val(val), m_type(Type::Double) {}

Val::Val(const std::string& str)
    : m_val(str), m_type(Type::String) {}

Val::Val(const char* str)
    : m_val(std::string(str)), m_type(Type::String) {}

Val::Val(const io::path_t& path)
    : m_val(path.toStdString()), m_type(Type::String) {}

Val::Val(const ValList& list)
    : m_val(list), m_type(Type::List) {}

Val::Val(const ValMap& map)
    : m_val(map), m_type(Type::Map) {}

Val::Type Val::valueType() const
{
    // std::monostate, bool, int, int64_t, double, std::string, ValList, ValMap
    switch (m_val.index()) {
    case 0: return Type::Undefined;
    case 1: return Type::Bool;
    case 2: return Type::Int;
    case 3: return Type::Int64;
    case 4: return Type::Double;
    case 5: return Type::String;
    case 6: return Type::List;
    case 7: return Type::Map;
    }
    return Type::Undefined;
}

void Val::setType(Type t)
{
    m_type = t;
}

Val::Type Val::type() const
{
    return m_type;
}

bool Val::isNull() const
{
    return m_val.index() == 0;
}

std::optional<bool> Val::doToBool() const
{
    switch (valueType()) {
    case Type::Bool:    return std::get<bool>(m_val);
    case Type::Int:     return muse::is_zero(std::get<int>(m_val)) ? false : true;
    case Type::Int64:   return muse::is_zero(std::get<int64_t>(m_val)) ? false : true;
    case Type::Double:  return muse::is_zero(std::get<double>(m_val)) ? false : true;
    case Type::String: {
        std::string str = std::get<std::string>(m_val);
        return !str.empty() && str != VAL_FALSE && str != "0" && str != "0.0";
    }
    case Type::List: return std::get<ValList>(m_val).size() > 0;
    case Type::Map: return std::get<ValMap>(m_val).size() > 0;
    default:
        break;
    }
    return std::nullopt;
}

bool Val::toBool() const
{
    auto value = doToBool();
    return value ? *value : false;
}

std::optional<int> Val::doToInt() const
{
    switch (valueType()) {
    case Type::Bool: return std::get<bool>(m_val) ? 1 : 0;
    case Type::Int: return std::get<int>(m_val);
    case Type::Int64: return static_cast<int>(std::get<int64_t>(m_val));
    case Type::Double: return static_cast<int>(std::get<double>(m_val));
    case Type::String: {
        std::string str = std::get<std::string>(m_val);
        if (str == VAL_TRUE) {
            return 1;
        }
        try {
            return std::stoi(str);
        } catch (const std::exception& e) {
            return std::nullopt;
        }
    }
    default:
        break;
    }
    return std::nullopt;
}

int Val::toInt() const
{
    auto value = doToInt();
    return value ? *value : 0;
}

std::optional<int64_t> Val::doToInt64() const
{
    switch (valueType()) {
    case Type::Bool: return std::get<bool>(m_val) ? 1 : 0;
    case Type::Int: return std::get<int>(m_val);
    case Type::Int64: return std::get<int64_t>(m_val);
    case Type::Double: return static_cast<int64_t>(std::get<double>(m_val));
    case Type::String: {
        std::string str = std::get<std::string>(m_val);
        if (str == VAL_TRUE) {
            return 1LL;
        }
        try {
            return std::stoll(str);
        } catch (const std::exception& e) {
            return std::nullopt;
        }
    }
    default:
        break;
    }
    return std::nullopt;
}

int64_t Val::toInt64() const
{
    auto value = doToInt64();
    return value ? *value : 0;
}

std::optional<double> Val::doToDouble() const
{
    switch (valueType()) {
    case Type::Bool: return std::get<bool>(m_val) ? 1.0 : 0.0;
    case Type::Int: return static_cast<double>(std::get<int>(m_val));
    case Type::Int64: return static_cast<double>(std::get<int64_t>(m_val));
    case Type::Double: return std::get<double>(m_val);
    case Type::String: {
        std::string str = std::get<std::string>(m_val);
        if (str == VAL_TRUE) {
            return 1.0;
        }
        try {
            return std::stod(str);
        } catch (const std::exception& e) {
            return std::nullopt;
        }
    }
    default:
        break;
    }
    return std::nullopt;
}

double Val::toDouble() const
{
    auto value = doToDouble();
    return value ? *value : 0.0;
}

float Val::toFloat() const
{
    return static_cast<float>(toDouble());
}

static std::string doubleToString(double n, int prec = 6)
{
    std::stringstream stream;
    stream << std::fixed << std::setprecision(prec) << n;
    std::string s = stream.str();

    // remove extra '0'
    size_t correctedIdx = s.size() - 1;
    if (s.back() == '0') {
        for (int i = static_cast<int>(s.size() - 1); i > 0; --i) {
            if (s.at(i) != '0') {
                correctedIdx = i;
                break;
            }
        }
    }

    if (s.at(correctedIdx) == '.') {
        --correctedIdx;
    }

    return std::string(s.c_str(), correctedIdx + 1);
}

std::optional<std::string> Val::doToString() const
{
    switch (valueType()) {
    case Type::Bool: return toBool() ? VAL_TRUE : VAL_FALSE;
    case Type::Int: return std::to_string(std::get<int>(m_val));
    case Type::Int64: return std::to_string(std::get<int64_t>(m_val));
    case Type::Double: return doubleToString(std::get<double>(m_val));
    case Type::String: return std::get<std::string>(m_val);
    case Type::List: {
        std::string str;
        if (!ValJson::toString(std::get<ValList>(m_val), str)) {
            return std::nullopt;
        }
        return str;
    }
    case Type::Map: {
        std::string str;
        if (!ValJson::toString(std::get<ValMap>(m_val), str)) {
            return std::nullopt;
        }
        return str;
    }
    default:
        break;
    }
    return std::nullopt;
}

std::string Val::toString() const
{
    auto value = doToString();
    return value ? *value : std::string();
}

std::optional<io::path_t> Val::doToPath() const
{
    if (valueType() == Type::String) {
        return std::get<std::string>(m_val);
    }
    return std::nullopt;
}

io::path_t Val::toPath() const
{
    auto value = doToPath();
    return value ? *value : io::path_t();
}

std::optional<ValList> Val::doToList() const
{
    switch (valueType()) {
    case Type::List: return std::get<ValList>(m_val);
    case Type::String: {
        ValList list;
        if (!ValJson::fromString(std::get<std::string>(m_val), list)) {
            return std::nullopt;
        }
        return list;
    }
    default:
        break;
    }

    return std::nullopt;
}

ValList Val::toList() const
{
    auto value = doToList();
    return value ? *value : ValList();
}

std::optional<ValMap> Val::doToMap() const
{
    switch (valueType()) {
    case Type::Map: return std::get<ValMap>(m_val);
    case Type::String: {
        ValMap map;
        if (!ValJson::fromString(std::get<std::string>(m_val), map)) {
            return std::nullopt;
        }
        return map;
    }
    default:
        break;
    }
    return std::nullopt;
}

ValMap Val::toMap() const
{
    auto value = doToMap();
    return value ? *value : ValMap();
}

bool Val::operator ==(const Val& v) const
{
    return v.m_type == m_type && v.m_val == m_val;
}

bool Val::operator <(const Val& v) const
{
    if (m_val.index() != v.m_val.index()) {
        return false;
    }

    return m_val < v.m_val;
}

#ifndef NO_QT_SUPPORT
Val::Val(const QString& str)
    : m_val(str.toStdString()), m_type(Type::String) {}

Val::Val(const QColor& color)
    : m_val(color.name().toStdString()), m_type(Type::Color) {}

QString Val::toQString() const
{
    return QString::fromStdString(toString());
}

QColor Val::toQColor() const
{
    std::string str = toString();
    return QColor(str.c_str());
}

QVariant Val::toQVariant() const
{
    switch (m_type) {
    case Val::Type::Undefined: return QVariant();
    case Val::Type::Bool: return QVariant(toBool());
    case Val::Type::Int: return QVariant(toInt());
    case Val::Type::Int64: return QVariant(static_cast<qlonglong>(toInt64()));
    case Val::Type::Double: return QVariant(toDouble());
    case Val::Type::String: return QVariant(toQString());
    case Val::Type::Color: return QVariant(toQColor());
    case Val::Type::List: {
        QVariantList vl;
        ValList l = toList();
        for (const Val& v : l) {
            vl << v.toQVariant();
        }
        return vl;
    }
    case Val::Type::Map: {
        QVariantMap vm;
        ValMap m = toMap();
        for (const auto& p : m) {
            vm.insert(QString::fromStdString(p.first), p.second.toQVariant());
        }
        return vm;
    }
    }
    return QVariant();
}

Val Val::fromQVariant(const QVariant& var)
{
    if (!var.isValid()) {
        return Val();
    }

    switch (var.typeId()) {
    case QMetaType::Bool: return Val(var.toBool());
    case QMetaType::Int: return Val(var.toInt());
    case QMetaType::UInt: return Val(var.toInt());
    case QMetaType::LongLong: return Val(static_cast<int64_t>(var.toLongLong()));
    case QMetaType::ULongLong: return Val(static_cast<int64_t>(var.toLongLong()));
    case QMetaType::Double: return Val(var.toDouble());
    case QMetaType::QString: return Val(var.toString().toStdString());
    case QMetaType::QVariantList:
    case QMetaType::QStringList: {
        ValList l;
        QVariantList vl = var.toList();
        for (const QVariant& v : vl) {
            l.push_back(fromQVariant(v));
        }
        return Val(l);
    }
    case QMetaType::QVariantMap: {
        ValMap m;
        QVariantMap vm = var.toMap();
        QVariantMap::const_iterator i = vm.constBegin();
        while (i != vm.constEnd()) {
            m.insert({ i.key().toStdString(), fromQVariant(i.value()) });
            ++i;
        }
        return Val(m);
    }
    case QMetaType::QColor: return Val(var.value<QColor>());
    default: {
        LOGE() << "Not supported type: " << var.typeName();
        //UNREACHABLE;
        return Val();
    }
    }
}

#endif // NO_QT_SUPPORT

static void appendQuoted(std::ostream& out, std::string_view text)
{
    static const char* hex = "0123456789abcdef";
    out << '"';
    for (size_t i = 0; i < text.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        switch (c) {
        case '"': out << "\\\"";
            break;
        case '\\': out << "\\\\";
            break;
        case '\b': out << "\\b";
            break;
        case '\f': out << "\\f";
            break;
        case '\n': out << "\\n";
            break;
        case '\r': out << "\\r";
            break;
        case '\t': out << "\\t";
            break;
        default:
            if (c < 0x20) {
                out << "\\u00" << hex[c >> 4] << hex[c & 0x0f];
            } else {
                out << static_cast<char>(c);
            }
            break;
        }
    }
    out << '"';
}

static bool appendValue(std::ostream& out, const Val& value)
{
    const std::optional<std::string> text = value.toType<std::string>();
    if (!text) {
        return false;
    }
    if (value.type() == Val::Type::String) {
        appendQuoted(out, *text);
        return true;
    }
    out << *text;
    return true;
}

static int hexDigit(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

static std::optional<std::string> unquote(std::string_view token)
{
    if (token.size() < 2 || token.front() != '"') {
        return std::nullopt;
    }

    std::string result;
    bool escaped = false;
    for (size_t i = 1; i < token.size(); ++i) {
        const char c = token[i];
        if (escaped) {
            switch (c) {
            case '"':
            case '\\':
            case '/':
                result.push_back(c);
                break;
            case 'b': result.push_back('\b');
                break;
            case 'f': result.push_back('\f');
                break;
            case 'n': result.push_back('\n');
                break;
            case 'r': result.push_back('\r');
                break;
            case 't': result.push_back('\t');
                break;
            case 'u': {
                if (i + 4 >= token.size()) {
                    return std::nullopt;
                }
                int code = 0;
                for (int h = 0; h < 4; ++h) {
                    const int digit = hexDigit(token[i + 1 + h]);
                    if (digit < 0) {
                        return std::nullopt;
                    }
                    code = (code << 4) | digit;
                }
                if (code >= 0xd800 && code <= 0xdfff) {
                    return std::nullopt;
                }
                if (code < 0x80) {
                    result.push_back(static_cast<char>(code));
                } else if (code < 0x800) {
                    result.push_back(static_cast<char>(0xc0 | (code >> 6)));
                    result.push_back(static_cast<char>(0x80 | (code & 0x3f)));
                } else {
                    result.push_back(static_cast<char>(0xe0 | (code >> 12)));
                    result.push_back(static_cast<char>(0x80 | ((code >> 6) & 0x3f)));
                    result.push_back(static_cast<char>(0x80 | (code & 0x3f)));
                }
                i += 4;
                break;
            }
            default:
                return std::nullopt;
            }
            escaped = false;
            continue;
        }
        if (c == '\\') {
            escaped = true;
            continue;
        }
        if (c == '"') {
            if (i != token.size() - 1) {
                return std::nullopt;
            }
            return result;
        }
        result.push_back(c);
    }
    return std::nullopt;
}

//! NOTE Quoted text is not structural: commas, colons and brackets inside it are data.
static bool skipQuoted(bool& inString, bool& escaped, char c)
{
    if (inString) {
        if (escaped) {
            escaped = false;
            return true;
        }
        if (c == '\\') {
            escaped = true;
            return true;
        }
        if (c == '"') {
            inString = false;
        }
        return true;
    }
    if (c == '"') {
        inString = true;
        return true;
    }
    return false;
}

bool ValJson::toString(const ValList& list, std::string& str)
{
    std::stringstream stream;
    stream << "[";
    for (size_t i = 0; i < list.size(); ++i) {
        if (!appendValue(stream, list.at(i))) {
            return false;
        }
        if (i + 1 < list.size()) {
            stream << ", ";
        }
    }
    stream << "]";
    str = stream.str();
    return true;
}

bool ValJson::toString(const ValMap& map, std::string& str)
{
    std::stringstream stream;
    stream << "{";
    std::string sep;
    for (const auto& item : map) {
        stream << sep;
        appendQuoted(stream, item.first);
        stream << ": ";
        if (!appendValue(stream, item.second)) {
            return false;
        }
        sep = ", ";
    }
    stream << "}";
    str = stream.str();
    return true;
}

bool ValJson::fromString(const std::string& str, ValList& list)
{
    return parseList(str, list);
}

bool ValJson::fromString(const std::string& str, ValMap& map)
{
    return parseMap(str, map);
}

std::string_view ValJson::trim(std::string_view str)
{
    while (!str.empty() && std::isspace(static_cast<unsigned char>(str.front()))) {
        str.remove_prefix(1);
    }
    while (!str.empty() && std::isspace(static_cast<unsigned char>(str.back()))) {
        str.remove_suffix(1);
    }
    return str;
}

bool ValJson::isIntegerToken(std::string_view str)
{
    if (str.empty()) {
        return false;
    }
    size_t i = (str.front() == '-' || str.front() == '+') ? 1 : 0;
    if (i == str.size()) {
        return false;
    }
    for (; i < str.size(); ++i) {
        if (!std::isdigit(static_cast<unsigned char>(str[i]))) {
            return false;
        }
    }
    return true;
}

bool ValJson::isDoubleToken(std::string_view str)
{
    if (str.empty()) {
        return false;
    }
    size_t i = (str.front() == '-' || str.front() == '+') ? 1 : 0;
    bool hasDot = false;
    bool hasDigit = false;
    for (; i < str.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(str[i]);
        if (std::isdigit(c)) {
            hasDigit = true;
        } else if (str[i] == '.' && !hasDot) {
            hasDot = true;
        } else {
            return false;
        }
    }
    return hasDot && hasDigit;
}

bool ValJson::splitTopLevel(std::string_view str, std::vector<std::string_view>& out)
{
    out.clear();
    if (str.empty()) {
        return true;
    }

    int depth = 0;
    bool inString = false;
    bool escaped = false;
    size_t start = 0;
    for (size_t i = 0; i < str.size(); ++i) {
        const char c = str[i];
        if (skipQuoted(inString, escaped, c)) {
            continue;
        }
        if (c == '[' || c == '{') {
            ++depth;
        } else if (c == ']' || c == '}') {
            if (depth == 0) {
                return false;
            }
            --depth;
        } else if (c == ',' && depth == 0) {
            out.push_back(str.substr(start, i - start));
            start = i + 1;
        }
    }

    if (inString || escaped || depth != 0) {
        return false;
    }

    out.push_back(str.substr(start));
    return true;
}

size_t ValJson::topLevelColon(std::string_view str)
{
    int depth = 0;
    bool inString = false;
    bool escaped = false;
    for (size_t i = 0; i < str.size(); ++i) {
        const char c = str[i];
        if (skipQuoted(inString, escaped, c)) {
            continue;
        }
        if (c == '[' || c == '{') {
            ++depth;
        } else if (c == ']' || c == '}') {
            if (depth == 0) {
                return std::string_view::npos;
            }
            --depth;
        } else if (c == ':' && depth == 0) {
            return i;
        }
    }
    return std::string_view::npos;
}

std::optional<Val> ValJson::parseValue(std::string_view token)
{
    token = trim(token);
    if (token.empty()) {
        return std::nullopt;
    }

    if (token.front() == '"') {
        auto text = unquote(token);
        if (!text) {
            return std::nullopt;
        }
        return Val(*text);
    }

    if (token.front() == '[') {
        ValList list;
        if (!parseList(token, list)) {
            return std::nullopt;
        }
        return Val(list);
    }

    if (token.front() == '{') {
        ValMap map;
        if (!parseMap(token, map)) {
            return std::nullopt;
        }
        return Val(map);
    }

    if (token == VAL_TRUE) {
        return Val(true);
    }
    if (token == VAL_FALSE) {
        return Val(false);
    }

    const std::string text(token);
    if (isIntegerToken(token)) {
        try {
            size_t consumed = 0;
            const long long value = std::stoll(text, &consumed);
            if (consumed != text.size()) {
                return std::nullopt;
            }
            if (value >= std::numeric_limits<int>::min() && value <= std::numeric_limits<int>::max()) {
                return Val(static_cast<int>(value));
            }
            return Val(static_cast<int64_t>(value));
        } catch (const std::exception&) {
            return Val(text);
        }
    }

    if (isDoubleToken(token)) {
        try {
            size_t consumed = 0;
            const double value = std::stod(text, &consumed);
            if (consumed != text.size()) {
                return std::nullopt;
            }
            return Val(value);
        } catch (const std::exception&) {
            return Val(text);
        }
    }

    return Val(text);
}

bool ValJson::parseList(std::string_view str, ValList& list)
{
    str = trim(str);
    if (str.size() < 2 || str.front() != '[' || str.back() != ']') {
        return false;
    }

    std::vector<std::string_view> parts;
    if (!splitTopLevel(trim(str.substr(1, str.size() - 2)), parts)) {
        return false;
    }

    ValList result;
    result.reserve(parts.size());
    for (const std::string_view part : parts) {
        auto value = parseValue(part);
        if (!value) {
            return false;
        }
        result.push_back(*value);
    }

    list = std::move(result);
    return true;
}

bool ValJson::parseMap(std::string_view str, ValMap& map)
{
    str = trim(str);
    if (str.size() < 2 || str.front() != '{' || str.back() != '}') {
        return false;
    }

    std::vector<std::string_view> parts;
    if (!splitTopLevel(trim(str.substr(1, str.size() - 2)), parts)) {
        return false;
    }

    ValMap result;
    for (const std::string_view part : parts) {
        const std::string_view entry = trim(part);
        const size_t colon = topLevelColon(entry);
        if (colon == std::string_view::npos) {
            return false;
        }

        const std::string_view rawKey = trim(entry.substr(0, colon));
        if (rawKey.empty()) {
            return false;
        }
        std::string key;
        if (rawKey.front() == '"') {
            auto parsedKey = unquote(rawKey);
            if (!parsedKey) {
                return false;
            }
            key = std::move(*parsedKey);
        } else {
            key = std::string(rawKey);
        }

        auto value = parseValue(entry.substr(colon + 1));
        if (!value) {
            return false;
        }
        result[std::move(key)] = *value;
    }

    map = std::move(result);
    return true;
}
