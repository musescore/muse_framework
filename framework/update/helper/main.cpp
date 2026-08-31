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

//! NOTE: Standalone, zero-runtime-dependency update helper. It runs from outside
//! the install location so that updating it never touches a file it is
//! executing.
//!
//! How the update is applied differs enough between platforms to be two separate
//! programs sharing an executable rather than one program with branches:
//!
//!  - where the install location is writable by the user running the
//!    application, the helper swaps it for the freshly unpacked update itself
//!    (swap.h);
//!  - on Windows the application is installed per-machine and no unprivileged
//!    process can replace it, so the helper instead runs as SYSTEM from a
//!    scheduled task and installs a signed package (updatetask_win.h).
//!
//! Only the one belonging to the platform is built.

#ifdef _WIN32
#include "updatetask_win.h"
#else
#include "swap.h"
#endif

int main(int argc, char** argv)
{
#ifdef _WIN32
    (void)argc;
    (void)argv;
    return updatetask::runCommandLine();
#else
    return swapper::run(argc, argv);
#endif
}
