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
//!                     [--install-args <args>]
//!                     [--upgrade-code <guid>] [--product-version <version>]
//!                     [--cert-from <signed file>] [--cert-subject <subject>]
//!                     [--cert-keys <sha256...>] [--cert-roots <sha256...>]
//!
//! What an update is judged against is registered here: the product it has to
//! be (`--upgrade-code`), the version it has to beat (`--product-version`) and
//! who has to have signed it. The certificate anchors are read out of
//! `--cert-from` - the package doing the registering - unless given explicitly,
//! which is what makes it possible to rotate a key: list the new hash alongside
//! the old one in a release, and only sign with it once that release is out.
//! Several values are separated by "|".
//!   --unregister-task --app-id <id>
//!   --apply           --app-id <id>   (the action of the scheduled task)
//!   --apply-run       --app-id <id>   (internal: the detached copy doing the work)
//!
//! The arguments are read from `GetCommandLineW` rather than from `argv`, which
//! cannot represent paths outside the ANSI code page.
int runCommandLine();
}
