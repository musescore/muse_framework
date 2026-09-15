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
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# The minimum macOS version that the produced binaries will run on.
#
# This file must be included before the `project()` command: `project()` initialises
# CMAKE_OSX_DEPLOYMENT_TARGET, and already uses it for the compiler checks it performs.
#
# The default set here can be overridden by specifying CMAKE_OSX_DEPLOYMENT_TARGET on the
# command line or in a CMake preset, or by setting the MACOSX_DEPLOYMENT_TARGET environment
# variable, which CMake picks up by itself.

# If the variable is not set by the time `project()` runs, CMake creates an empty cache entry
# for it; FORCE makes sure that such an empty value is replaced by the default below.

if(APPLE AND NOT CMAKE_OSX_DEPLOYMENT_TARGET AND NOT DEFINED ENV{MACOSX_DEPLOYMENT_TARGET})
    set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15.4 CACHE STRING "Minimum macOS version to target for deployment" FORCE)
endif()
