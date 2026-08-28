/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore BVBA and others
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

#include "interactive/iplatforminteractive.h"

namespace muse {
class PlatformInteractiveMock : public IPlatformInteractive
{
public:
    MOCK_METHOD(Ret, openUrl, (const std::string& url), (const, override));
    MOCK_METHOD(Ret, openUrl, (const QUrl& url), (const, override));

    MOCK_METHOD(Ret, isAppExists, (const std::string& appIdentifier), (const, override));
    MOCK_METHOD(Ret, canOpenApp, (const UriQuery& uri), (const, override));
    MOCK_METHOD(async::Promise<Ret>, openApp, (const UriQuery& uri), (const, override));

    MOCK_METHOD(Ret, revealInFileBrowser, (const io::path_t& filePath), (const, override));
};
}
