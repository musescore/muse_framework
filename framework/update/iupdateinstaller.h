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
#ifndef MUSE_UPDATE_IUPDATEINSTALLER_H
#define MUSE_UPDATE_IUPDATEINSTALLER_H

#include "types/ret.h"
#include "io/path.h"

#include "modularity/imoduleinterface.h"

namespace muse::update {
class IUpdateInstaller : MODULE_GLOBAL_INTERFACE
{
    INTERFACE_ID(IUpdateInstaller)

public:
    virtual ~IUpdateInstaller() = default;

    //! Whether this platform can apply an update in-place (replace the install
    //! location and restart) AND the install location is writable by the
    //! current user. When false, callers should fall back to opening the
    //! downloaded installer for the user to run manually.
    virtual bool isInPlaceUpdateSupported() const = 0;

    //! Unpack `packagePath`, spawn the standalone helper that replaces the
    //! current install location once this process exits, and relaunches the
    //! application. Returns OK if the helper was successfully spawned; the
    //! caller must then quit the application.
    virtual Ret applyUpdate(const muse::io::path_t& packagePath) = 0;
};
}

#endif // MUSE_UPDATE_IUPDATEINSTALLER_H
