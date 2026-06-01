/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore/Audacity CLA applies
 *
 * Copyright (C) 2026 MuseScore/Audacity and others
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

#include <string>

#include "global/serialization/json.h"

namespace muse::rcontrol::mcp {
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

inline std::string to_string(DataType type)
{
    switch (type) {
    case DataType::Undefined: return "undefined";
    case DataType::String: return "string";
    case DataType::Integer: return "integer";
    case DataType::Float: return "number";
    case DataType::Boolean: return "boolean";
    case DataType::Object: return "object";
    case DataType::Array: return "array";
    case DataType::Null: return "null";
    default: return "undefined";
    }
}

struct Property {
    DataType type;
    std::string description;
    JsonValue minimum;
    JsonValue maximum;
};

struct InputSchema {
    std::string type = "object";
    std::map<std::string, Property> properties;
};

struct Tool {
    std::string name;
    std::string title;
    std::string description;
    InputSchema inputSchema;
};

struct ToolResult {
    bool isError = false;
};
}
