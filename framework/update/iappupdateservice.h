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

    virtual bool isReleaseDownloaded() const = 0;
    virtual muse::io::path_t downloadedReleasePath() const = 0;

    virtual bool canAutoInstall() const = 0;

    virtual RetVal<muse::io::path_t> prepareUpdate(const muse::io::path_t& packagePath) = 0;
    virtual Ret finalizeUpdate(const muse::io::path_t& preparedPath) = 0;
};
}
