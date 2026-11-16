# SPDX-License-Identifier: GPL-3.0-only
# MuseScore-CLA-applies
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

set(MODULE_TEST muse_dockwindow_tests)

set(MODULE_TEST_SRC
    ${CMAKE_CURRENT_LIST_DIR}/topleveltoolbarslayout_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dockwindow_tests.cpp
    )

get_property(kddockwidgets_src_dir GLOBAL PROPERTY kddockwidgets_SOURCE_DIR)

set(MODULE_TEST_INCLUDE
    ${CMAKE_CURRENT_LIST_DIR}/..
    ${CMAKE_CURRENT_LIST_DIR}/../qml/Muse/Dock
    ${kddockwidgets_src_dir}
    ${kddockwidgets_src_dir}/kddockwidgets/src
    )

set(MODULE_TEST_LINK
    muse_dockwindow
    muse_rcommand
    muse_dockwindow_qml
    muse_ui_qml
    muse_uicomponents_qml
    kddockwidgets
    Qt::Qml
    Qt::Quick
    )

include(SetupGTest)
