/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

namespace muse::audio {
//! NOTE macOS prevents the system from going to idle sleep for as long as a process has
//! audio IO running, no matter whether that IO is audible. Since we keep the audio device
//! open for the whole session, that would keep the Mac awake permanently, also when idle.
//! So we tell CoreAudio that idle sleep is fine, and only opt out of it while something
//! is actually being played back.
//!
//! The policy applies to the process as a whole, so it is shared by all drivers.
class OSXIdleSleepPolicy
{
public:
    static void setPreventIdleSleep(bool prevent);
};
}
