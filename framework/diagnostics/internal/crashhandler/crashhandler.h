/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
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

#ifndef MUSE_DIAGNOSTICS_CRASHHANDLER_H
#define MUSE_DIAGNOSTICS_CRASHHANDLER_H

#include <unordered_map>
#include <string>

#include "io/path.h"
#include "modularity/ioc.h"
#include "global/iapplication.h"
#include "global/io/ifilesystem.h"

#include "diagnostics/icrashhandler.h"

namespace crashpad {
class CrashpadClient;
}

namespace muse::diagnostics {
class CrashHandler : public ICrashHandler
{
    GlobalInject<muse::io::IFileSystem> fileSystem;
    GlobalInject<muse::IApplication> application;

public:
    CrashHandler() = default;
    ~CrashHandler() override;

    bool start(const muse::io::path_t& handlerFilePath, const muse::io::path_t& dumpsDir, const std::string& serverUrl);

    void addSessionTag(const String& tag, const String& value) override;
    void setSystemCrashReporterForwardingEnabled(bool enabled) override;

private:
    void removePendingLockFiles(const muse::io::path_t& dumpsDir);

    std::unordered_map<muse::String, String> m_sessionTags;
    crashpad::CrashpadClient* m_client = nullptr;
};
}

#endif // MUSE_DIAGNOSTICS_CRASHHANDLER_H
