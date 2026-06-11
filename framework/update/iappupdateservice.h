/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2025 MuseScore Limited and others
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

#include "types/retval.h"
#include "async/promise.h"
#include "progress.h"

#include "updatetypes.h"

#include "modularity/imoduleinterface.h"

namespace muse::update {
class IAppUpdateService : MODULE_CONTEXT_INTERFACE
{
    INTERFACE_ID(IAppUpdateService)

public:
    virtual ~IAppUpdateService() = default;

    virtual async::Promise<muse::RetVal<ReleaseInfo> > checkForUpdate() = 0;
    virtual const RetVal<ReleaseInfo>& lastCheckResult() const = 0;
    virtual RetVal<Progress> downloadRelease() = 0;

    //! Whether the downloaded release can be installed in-place automatically
    //! (replace the install location and restart) instead of handing the
    //! installer to the user.
    virtual bool canAutoInstall() const = 0;

    //! Apply a previously downloaded release in-place. Returns OK if the update
    //! was successfully staged; the caller must then quit the application.
    virtual Ret applyUpdate(const muse::io::path_t& packagePath) = 0;
};
}
