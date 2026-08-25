/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited and others
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

#include "rcommand/commandtypes.h"
#include "extensionstypes.h"

namespace muse::extensions {
inline static const muse::rcommand::Command OPEN_APIDUMP_COMMAND("command://extensions/open-apidump");

inline rcommand::Command makeCommand(const ExtensionUri& extensionUri, const ExtensionActionCode& action)
{
    // extension://some/uri + action -> command://extension/some/uri/action
    rcommand::Command c;
    c.setScheme(std::string(rcommand::COMMAND_SCHEME));
    c.addPath(std::string(EXTENSION_SCHEME)); // scheme as first segment of path
    c.addPath(extensionUri.path());
    c.addPath(action);
    return c;
}

inline ExtensionUri extensionUriByCommand(const rcommand::Command& c)
{
    // command://extension/some/uri/action -> extension://some/uri

    std::vector<std::string> segments = c.pathSegments();
    IF_ASSERT_FAILED(segments.size() >= 3) {
        return ExtensionUri();
    }
    IF_ASSERT_FAILED(segments.at(0) == std::string(EXTENSION_SCHEME)) {
        return ExtensionUri();
    }

    ExtensionUri uri;
    uri.setScheme(std::string(EXTENSION_SCHEME));
    for (size_t i = 1 /* skip extension */; i < (segments.size() - 1 /* skip action */); ++i) {
        uri.addPath(segments.at(i));
    }
    return uri;
}

inline ExtensionActionCode actionByCommand(const rcommand::Command& c)
{
    // command://extension/some/uri/action -> action
    std::vector<std::string> segments = c.pathSegments();
    IF_ASSERT_FAILED(segments.size() >= 3) {
        return {};
    }
    return segments.at(segments.size() - 1);
}
}
