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
#ifndef MUSE_DIAGNOSTICS_ICRASHHANDLER_H
#define MUSE_DIAGNOSTICS_ICRASHHANDLER_H

#include "modularity/imoduleinterface.h"
#include "global/types/string.h"

namespace muse::diagnostics {
//! Lets the application attach context to the crash reports of the running process.
//! Only registered when the crashpad client is built in (MUSE_MODULE_DIAGNOSTICS_CRASHPAD_CLIENT),
//! so check for null.
class ICrashHandler : MODULE_GLOBAL_INTERFACE
{
    INTERFACE_ID(ICrashHandler)
public:
    virtual ~ICrashHandler() = default;

    //! Adds a Sentry tag to the session, helpful for filtering.
    //! Must be called when applying command-line options - onInit is too late.
    virtual void addSessionTag(const String& tag, const String& value) = 0;

    //! Whether a crash of this process is also handed to the operating system's crash
    //! reporter. On by default. A spawned plugin-registration child process may want to turn it off.
    //! On macOS that reporter is what shows the "<app> quit unexpectedly" dialog.
    //! Windows and Linux have no such reporter, so this has no effect there.
    virtual void setSystemCrashReporterForwardingEnabled(bool enabled) = 0;
};
}

#endif // MUSE_DIAGNOSTICS_ICRASHHANDLER_H
