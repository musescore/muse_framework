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

# NOTE This is the remote control module
muse_create_module(muse_rcontrol ALIAS muse::rcontrol)

target_sources(muse_rcontrol PRIVATE
    rcontrolmodule.cpp
    rcontrolmodule.h
)

add_executable(musescore-mcpbridge
    mcpbridge/main.cpp
)

set_target_properties(musescore-mcpbridge PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

#if (MUSE_MODULE_RCONTROL_TESTS)
#    add_subdirectory(tests)
#endif()
