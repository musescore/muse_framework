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

#include <gmock/gmock.h>

#include "update/iappupdateservice.h"

namespace muse::update {
class AppUpdateServiceMock : public IAppUpdateService
{
public:
    MOCK_METHOD(async::Promise<muse::RetVal<ReleaseInfo> >, checkForUpdate, (), (override));
    MOCK_METHOD(const RetVal<ReleaseInfo>&, lastCheckResult, (), (const, override));
    MOCK_METHOD(RetVal<Progress>, downloadRelease, (), (override));

    MOCK_METHOD(bool, isReleaseDownloaded, (), (const, override));
    MOCK_METHOD(muse::io::path_t, downloadedReleasePath, (), (const, override));

    MOCK_METHOD(bool, canAutoInstall, (), (const, override));

    MOCK_METHOD(RetVal<muse::io::path_t>, prepareUpdate, (const muse::io::path_t&), (override));
    MOCK_METHOD(Ret, finalizeUpdate, (const muse::io::path_t&), (override));
};
}
