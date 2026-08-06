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

//! The privileged side of updating on Windows: sets up the scheduled task that
//! runs as SYSTEM, and, from inside that task, installs a downloaded package.
namespace updatetask {
//! Parses the command line, carries out the sub-command it names and returns the
//! process exit code. One of:
//!
//!   --register-task   --app-id <id> --app-exe <path relative to the install dir>
//!                     --install-dir <dir> [--package-type msi|exe]
//!                     [--install-args <args>] [--cert-subject <subject>]
//!   --unregister-task --app-id <id>
//!   --apply           --app-id <id>   (the action of the scheduled task)
//!   --apply-run       --app-id <id>   (internal: the detached copy doing the work)
//!
//! The arguments are read from `GetCommandLineW` rather than from `argv`, which
//! cannot represent paths outside the ANSI code page.
int runCommandLine();
}
