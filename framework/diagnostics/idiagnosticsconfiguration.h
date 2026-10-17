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
#ifndef MUSE_DIAGNOSTICS_IDIAGNOSTICSCONFIGURATION_H
#define MUSE_DIAGNOSTICS_IDIAGNOSTICSCONFIGURATION_H

#include <map>

#include "modularity/imoduleinterface.h"

#include "io/path.h"
#include "global/types/string.h"

namespace muse::diagnostics {
//! Crash dumps are laid in `directory` and then sent to `serverUrl` by a separate process. The process carries the URL information, not the dump.
//! Hence, a given dump dir should serve for a given server URL only, or a dump may not get uploaded to the intended URL.
//! Just setting a dumps directory is okay, though - dumps just don't get uploaded.
struct CrashDumpConfig {
    CrashDumpConfig(io::path_t directory, String serverUrl = String())
        : directory{std::move(directory)}, serverUrl{std::move(serverUrl)}
    {
    }

    const io::path_t directory;
    const String serverUrl;
};

class IDiagnosticsConfiguration : MODULE_GLOBAL_INTERFACE
{
    INTERFACE_ID(IDiagnosticsConfiguration)

public:
    virtual bool isDumpUploadAllowed() const = 0;
    virtual void setIsDumpUploadAllowed(bool val) = 0;

    virtual bool shouldWarnBeforeSavingDiagnosticFiles() const = 0;
    virtual void setShouldWarnBeforeSavingDiagnosticFiles(bool val) = 0;

    virtual muse::io::path_t diagnosticFilesDefaultSavingPath() const = 0;

    virtual CrashDumpConfig crashDumpConfig() const = 0;
    virtual void setCrashDumpConfig(const CrashDumpConfig& config) = 0;

    virtual std::map<String, String> crashReportTags() const = 0;
    virtual void setCrashReportTags(std::map<String, String> tags) = 0;

    //! Whether a crash of this process is also handed to the operating system's crash
    //! reporter. On macOS that reporter is what shows the "<app> quit unexpectedly" dialog.
    //! Windows and Linux have no such reporter, so this has no effect there.
    virtual bool systemCrashReporterForwardingEnabled() const = 0;
    virtual void setSystemCrashReporterForwardingEnabled(bool val) = 0;
};
}

#endif // MUSE_DIAGNOSTICS_IDIAGNOSTICSCONFIGURATION_H
