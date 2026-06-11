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
#ifndef MUSE_UPDATE_MACUPDATEINSTALLER_H
#define MUSE_UPDATE_MACUPDATEINSTALLER_H

#include "../../../iupdateinstaller.h"

#include "modularity/ioc.h"
#include "io/ifilesystem.h"
#include "../../../iupdateconfiguration.h"

namespace muse::update {
class MacUpdateInstaller : public IUpdateInstaller, public Contextable
{
    GlobalInject<io::IFileSystem> fileSystem;
    GlobalInject<IUpdateConfiguration> configuration;

public:
    MacUpdateInstaller(const modularity::ContextPtr& iocCtx)
        : Contextable(iocCtx) {}

    bool isInPlaceUpdateSupported() const override;
    Ret applyUpdate(const muse::io::path_t& packagePath) override;

private:
    //! Path to the running `*.app` bundle (the install location to replace).
    muse::io::path_t currentBundlePath() const;

    //! Path to the bundled `museupdater` helper (Contents/MacOS/museupdater).
    muse::io::path_t helperPath() const;
};
}

#endif // MUSE_UPDATE_MACUPDATEINSTALLER_H
