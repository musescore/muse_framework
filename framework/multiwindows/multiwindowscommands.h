/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-Studio-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) MuseScore Limited and others
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

#include "rcommand/commandtypes.h"

namespace muse::mi {
inline static const muse::rcommand::Command MULTIWINDOWS_DEV_SHOW_INFO_COMMAND("command://multiwindows/dev/show-info");

//! NOTE These are commands from the application; the mi module does not register or process them, but it does send them.
inline static const muse::rcommand::Command APP_QUIT_COMMAND("command://app/quit");
inline static const muse::rcommand::Command APP_RESTART_COMMAND("command://app/restart");
}
