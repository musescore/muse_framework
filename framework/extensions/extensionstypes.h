/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2024 MuseScore Limited and others
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

#include <string_view>
#include <vector>
#include <map>

#include "global/types/uri.h"
#include "global/types/string.h"
#include "global/types/val.h"
#include "global/io/path.h"
#include "global/types/translatablestring.h"
#include "ui/view/iconcodes.h"
#include "rcommand/commandtypes.h"

#include "log.h"

namespace muse::extensions {
//! NOTE Api versions:
//! 1 - plugins from 3х
//! 2 - extensions
constexpr int DEFAULT_API_VERSION = 2;

//! NOTE Default extension dialog modality
constexpr bool DEFAULT_MODAL = false;

constexpr const char* SINGLE_FILE_EXT = "mext";

// Contexts
constexpr const std::string_view ANY_CONTEXT = "Any";
constexpr const std::string_view PROJECT_OPENED_CONTEXT = "ProjectOpened";
constexpr const std::string_view DEFAULT_CONTEXT = PROJECT_OPENED_CONTEXT;

// Uri
constexpr std::string_view EXTENSION_SCHEME = "extension";

//! NOTE URI is used as an identifier
using ExtensionUri = Uri;
//! NOTE Action code is used to perform actions on the extension
using ExtensionActionCode = std::string;

enum class Type {
    Undefined = 0,
    Form,       // Have UI, controls, user interaction
    Macros,     // Without UI, they just do some script
    Composite   // Composite with some UI and script
};

static inline Type typeFromString(const std::string& str)
{
    if (str == "form") {
        return Type::Form;
    } else if (str == "macros") {
        return Type::Macros;
    } else if (str == "composite") {
        return Type::Composite;
    }
    return Type::Undefined;
}

static inline std::string typeToString(const Type& type)
{
    switch (type) {
    case Type::Undefined: return "undefined";
    case Type::Form: return "form";
    case Type::Macros: return "macros";
    case Type::Composite: return "composite";
    }
    return std::string();
}

enum Filter {
    Enabled,
    All
};

struct Action {
    ExtensionActionCode code;
    Type type = Type::Undefined;
    bool modal = DEFAULT_MODAL;
    String title;
    ui::IconCode::Code icon = ui::IconCode::Code::NONE;
    bool showOnToolbar = false;
    bool showOnAppmenu = true;
    io::path_t path;
    std::string func = "main";
    std::string context = std::string(DEFAULT_CONTEXT);
    int apiversion = DEFAULT_API_VERSION;
    bool legacyPlugin = false;

    bool isValid() const { return type != Type::Undefined && !code.empty(); }
};

/*
manifest.json
{

"uri": String,                    // Example: extension://module/target/name
"type": String,                   // Values: form, macros
"title": String,                  //
"description": String,            //
"category": String,               //
"thumbnail": String,              //
"version": String,                //
"apiversion": String,             // Optional default 2

"contributes": {                  // Optional map of contribution points
    String: [Object, ...]
},

"main": String                    // Path (name) of main file (qml or js)
}*/

struct Manifest {
    ExtensionUri uri;
    io::path_t path;
    Type type = Type::Undefined;
    String title;
    String description;
    String category;
    io::path_t thumbnail;
    std::string version;
    std::string context = std::string(DEFAULT_CONTEXT);
    int apiversion = DEFAULT_API_VERSION;
    bool legacyPlugin = false;
    bool isRemovable = false;

    std::map<std::string, std::vector<ValMap> > contributes;

    std::vector<Action> actions;

    bool isValid() const { return type != Type::Undefined && uri.isValid(); }

    Action action(const ExtensionActionCode& code) const
    {
        for (const Action& a : actions) {
            if (a.code == code) {
                return a;
            }
        }
        return {};
    }
};

using ManifestList = std::vector<Manifest>;

using KnownCategories = std::map<std::string /*name*/, TranslatableString /*title*/>;

struct ExtensionConfig {
    bool enabled = false;
};
}
