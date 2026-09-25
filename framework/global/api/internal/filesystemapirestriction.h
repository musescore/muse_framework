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

#include "../ifilesystemapirestriction.h"

#include <string>
#include <map>

namespace muse::api {
class FileSystemApiRestriction : public IFileSystemApiRestriction
{
public:

    void addAllowedPathBase(const std::string& key, const io::path_t& path) override;
    void removeAllowedPathBase(const std::string& key) override;
    void clearAllowedPathBases() override;

    Ret isPathAllowed(const io::path_t& path) const override;

private:
    //! Absolute path with symlinks resolved left to right.
    //! `..` is applied to the already resolved prefix. The not-yet-existing tail is appended as-is.
    //! Empty if the path cannot be resolved safely (loop, broken link with no target).
    static std::string resolvedPath(const io::path_t& path);

    std::map<std::string /* key */, std::string /* path */> m_allowedPathBases;
};
}
