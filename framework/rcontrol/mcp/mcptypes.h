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

#include "global/types/ret.h"
#include "global/types/val.h"
#include "global/types/flags.h"

namespace muse::rcontrol::mcp {
enum class DataType {
    Undefined = 0,
    String,
    Integer,
    Number,
    Boolean,
    Object,
    Array,
    Null
};

enum class Format {
    Undefined = 0,
    Int64,
    Float,
    Double
};

struct TypeSchema {
    DataType type = DataType::Undefined;
    Format format = Format::Undefined;
};

inline std::string to_string(DataType type)
{
    switch (type) {
    case DataType::Undefined: return "undefined";
    case DataType::String: return "string";
    case DataType::Integer: return "integer";
    case DataType::Number: return "number";
    case DataType::Boolean: return "boolean";
    case DataType::Object: return "object";
    case DataType::Array: return "array";
    case DataType::Null: return "null";
    }
    return "undefined";
}

inline std::string to_string(Format format)
{
    switch (format) {
    case Format::Undefined: return "undefined";
    case Format::Int64: return "int64";
    case Format::Float: return "float";
    case Format::Double: return "double";
    }
    return "undefined";
}

struct Property {
    TypeSchema type;
    std::string description;
    std::optional<ValList> enumValues;
    std::optional<Val> minimum;
    std::optional<Val> maximum;
};

struct PropertiesSchema {
    std::string type = "object";
    std::map<std::string, Property> properties;
    std::vector<std::string> required;
};

using InputSchema = PropertiesSchema;
using OutputSchema = PropertiesSchema;

struct Annotations
{
    bool readOnly = false;
    bool destructive = true;      // MCP: destructiveHint defaults to true
    bool idempotent = false;
    bool openWorld = true;        // MCP: openWorldHint defaults to true
};

struct Tool {
    std::string name;
    std::string title;
    std::string description;
    InputSchema inputSchema;
    OutputSchema outputSchema;
    Annotations annotations;
};

struct ToolResult {
    muse::Ret ret;
    std::map<std::string, Val> result;
};
}
