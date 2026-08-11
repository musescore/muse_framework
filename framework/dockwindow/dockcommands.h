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

namespace muse::dock {
inline static const muse::rcommand::Command DOCK_SET_OPEN_COMMAND("command://dock/set-open");
inline static const muse::rcommand::Command DOCK_TOGGLE_COMMAND("command://dock/toggle");
inline static const muse::rcommand::Command DOCK_TOGGLE_FLOATING_COMMAND("command://dock/toggle-floating");
inline static const muse::rcommand::Command DOCK_RESTORE_DEFAULT_LAYOUT_COMMAND("command://dock/restore-default-layout");
}
