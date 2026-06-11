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
#ifndef MUSE_UPDATE_UPDATEINSTALLERSTUB_H
#define MUSE_UPDATE_UPDATEINSTALLERSTUB_H

#include "../../../iupdateinstaller.h"

namespace muse::update {
//! Used on platforms that do not yet implement in-place updates. Callers fall
//! back to opening the downloaded installer for the user to run manually.
class UpdateInstallerStub : public IUpdateInstaller
{
public:
    bool isInPlaceUpdateSupported() const override;
    Ret applyUpdate(const muse::io::path_t& packagePath) override;
};
}

#endif // MUSE_UPDATE_UPDATEINSTALLERSTUB_H
