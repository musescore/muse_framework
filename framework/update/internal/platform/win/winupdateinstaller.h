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

#include <string>

#include "../../../iupdateinstaller.h"

#include "modularity/ioc.h"
#include "io/ifilesystem.h"
#include "../../../iupdateconfiguration.h"

namespace muse::update {
//! NOTE: The application is installed per-machine (Program Files), which an
//! ordinary user cannot write to. Rather than prompting for elevation, the
//! installer registers a scheduled task that runs as SYSTEM on demand; this
//! class hands the downloaded package to that task, which installs it silently.
//! If the task is missing (portable build, task removed by policy), in-place
//! update is reported as unsupported and the caller falls back to letting the
//! user run the installer manually.
class WinUpdateInstaller : public IUpdateInstaller, public Contextable
{
    GlobalInject<io::IFileSystem> fileSystem;
    GlobalInject<IUpdateConfiguration> configuration;

public:
    WinUpdateInstaller(const modularity::ContextPtr& iocCtx)
        : Contextable(iocCtx) {}

    bool isInPlaceUpdateSupported() const override;
    Ret applyUpdate(const muse::io::path_t& packagePath) override;

private:
    //! Identifier shared with the installer-registered task and the HKLM key;
    //! the base name of the running executable, e.g. "MuseScore4".
    std::wstring appId() const;
};
}
