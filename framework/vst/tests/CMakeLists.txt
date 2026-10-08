# SPDX-License-Identifier: GPL-3.0-only
# MuseScore-CLA-applies
#
# MuseScore Studio
# Music Composition & Notation
#
# Copyright (C) 2025 MuseScore Limited and others
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

set(MODULE_TEST muse_vst_tests)

# The vst headers include their siblings unprefixed (vstsequencer.h -> "vsttypes.h"), so the module
# directory must be on the include path, as it is when the module itself is built.
set(MODULE_TEST_INCLUDE ${CMAKE_CURRENT_LIST_DIR}/..)

set(MODULE_TEST_SRC
    ${CMAKE_CURRENT_LIST_DIR}/vstsequencertest.cpp
)

# muse_vst pulls in its QML view (VstViewDialog), so the test links the UI stack it needs.
set(MODULE_TEST_LINK muse_vst muse_audio muse_uicomponents muse_ui muse_ui_qml muse_mpe vst3sdk Qt::Quick Qt::Widgets)

include(SetupGTest)
