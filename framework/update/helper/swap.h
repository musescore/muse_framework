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

namespace swapper {
//! Waits for the host application to exit, stages the freshly unpacked update
//! next to the install location, verifies it, atomically swaps it into place
//! and relaunches the application.
//!
//!   --wait-pid <pid> --src <dir> --dst <dir> [--relaunch <path>] [--log <path>]
//!
//! Used where the install location is writable by the user running the
//! application, which on Windows it is not - see updatetask_win.h for that.
int run(int argc, char** argv);
}
