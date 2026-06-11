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

set(UPDATE_HELPER_TARGET museupdater)

set(UPDATE_HELPER_SRC
    main.cpp
    platform.h
)

if (OS_IS_MAC)
    list(APPEND UPDATE_HELPER_SRC platform_mac.cpp)
elseif (OS_IS_WIN)
    list(APPEND UPDATE_HELPER_SRC platform_win.cpp)
else()
    list(APPEND UPDATE_HELPER_SRC platform_unix.cpp)
endif()

add_executable(${UPDATE_HELPER_TARGET} ${UPDATE_HELPER_SRC})

set_target_properties(${UPDATE_HELPER_TARGET} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# On macOS the helper is embedded into the app bundle by the app target.
if (NOT OS_IS_MAC)
    install(TARGETS ${UPDATE_HELPER_TARGET}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()
