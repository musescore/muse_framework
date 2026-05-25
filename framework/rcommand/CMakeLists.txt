# SPDX-License-Identifier: GPL-3.0-only
# MuseScore-Studio-CLA-applies
#
# MuseScore Studio
# Music Composition & Notation
#
# Copyright (C) 2026 MuseScore Limited and others
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 3 as
# published by the Free Software Foundation.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <https://www.gnu.org/licenses/>.

muse_create_module(muse_rcommand ALIAS muse::rcommand)

target_sources(muse_rcommand PRIVATE
    rcommandmodule.cpp
    rcommandmodule.h
    ircommanddispatcher.h
    rcommandable.h
    rcommandtypes.h

    internal/rcommanddispatcher.cpp
    internal/rcommanddispatcher.h
)

#if (MUSE_MODULE_RCOMMAND_TESTS)
#    add_subdirectory(tests)
#endif()
