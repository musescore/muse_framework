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

#include "../../../iupdateinstaller.h"

#include "modularity/ioc.h"
#include "io/ifilesystem.h"
#include "../../../iupdateconfiguration.h"

namespace muse::update {
//! NOTE: The whole application is a single AppImage file, so an update is
//! nothing more than replacing that file - the downloaded package needs no
//! unpacking. This only applies when we are actually running from an AppImage;
//! distribution packages, Flatpak and Snap manage their own updates and a plain
//! build has no single file to replace, so in-place update is reported as
//! unsupported there and the caller falls back to handing the download to the
//! user.
class LinuxUpdateInstaller : public IUpdateInstaller, public Contextable
{
    GlobalInject<io::IFileSystem> fileSystem;
    GlobalInject<IUpdateConfiguration> configuration;

public:
    LinuxUpdateInstaller(const modularity::ContextPtr& iocCtx)
        : Contextable(iocCtx) {}

    bool isInPlaceUpdateSupported() const override;
    Ret applyUpdate(const muse::io::path_t& packagePath) override;

private:
    //! Path to the running AppImage file (the install location to replace), or
    //! empty when not running from an AppImage. Symlinks are resolved, so that
    //! launching through e.g. `~/.local/bin/mscore` replaces the real file and
    //! leaves the symlink pointing at it.
    muse::io::path_t currentAppImagePath() const;

    //! Path to the `museupdater` helper bundled next to the application binary.
    muse::io::path_t helperPath() const;
};
}
