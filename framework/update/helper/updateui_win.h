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

#include <string>

//! NOTE: The progress window shown while the package is installing.
//!
//! It is a separate process rather than a window of the privileged side: the
//! installation runs as SYSTEM from a scheduled task, in session 0, where
//! nothing it draws would ever reach a screen. This half is started by that
//! privileged side in the session of the user who asked for the update, with no
//! privileges of its own, and is driven over a pipe.
namespace updateui {
namespace command {
inline const char* TITLE = "title";
inline const char* MESSAGE = "message";
inline const char* BACKGROUND = "background";
inline const char* ACCENT = "accent";
inline const char* FOREGROUND = "foreground";

inline const char* SHOW = "show";

inline const char* PROGRESS = "progress";

inline const char* CLOSE = "close";
}

int run(const std::wstring& pipeHandleValue);
}
