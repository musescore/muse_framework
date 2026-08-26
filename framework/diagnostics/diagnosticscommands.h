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

namespace muse::diagnostics {
inline static const muse::rcommand::Command DIAGNOSTICS_SAVE_FILES_COMMAND("command://diagnostics/save-files");
inline static const muse::rcommand::Command DIAGNOSTICS_SHOW_PATHS_COMMAND("command://diagnostics/show-paths");
inline static const muse::rcommand::Command DIAGNOSTICS_SHOW_PROFILER_COMMAND("command://diagnostics/show-profiler");
inline static const muse::rcommand::Command DIAGNOSTICS_SHOW_GRAPHICSINFO_COMMAND("command://diagnostics/show-graphicsinfo");
inline static const muse::rcommand::Command DIAGNOSTICS_SHOW_NAVIGATION_TREE_COMMAND("command://diagnostics/show-navigation-tree");
inline static const muse::rcommand::Command DIAGNOSTICS_SHOW_ACCESSIBLE_TREE_COMMAND("command://diagnostics/show-accessible-tree");
inline static const muse::rcommand::Command DIAGNOSTICS_DUMP_ACCESSIBLE_TREE_COMMAND("command://diagnostics/dump-accessible-tree");
inline static const muse::rcommand::Command DIAGNOSTICS_SHOW_ENGRAVING_ELEMENTS_COMMAND("command://diagnostics/show-engraving-elements");
inline static const muse::rcommand::Command DIAGNOSTICS_SHOW_ENGRAVING_UNDOSTACK_COMMAND("command://diagnostics/show-engraving-undostack");
inline static const muse::rcommand::Command DIAGNOSTICS_SHOW_ENGRAVING_STYLE_COMMAND("command://diagnostics/show-engraving-style");
inline static const muse::rcommand::Command DIAGNOSTICS_SHOW_ACTIONS_COMMAND("command://diagnostics/show-actions");
inline static const muse::rcommand::Command DIAGNOSTICS_SHOW_RCOMMANDS_COMMAND("command://diagnostics/show-rcommands");
}
