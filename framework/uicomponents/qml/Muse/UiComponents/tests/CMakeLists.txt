# SPDX-License-Identifier: GPL-3.0-only
# MuseScore-CLA-applies
#
# MuseScore Studio
# Music Composition & Notation
#
# Copyright (C) 2021 MuseScore Limited and others
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

set(MODULE_TEST muse_uicomponents_qml_tests)

set(MODULE_TEST_SRC
    ${CMAKE_CURRENT_LIST_DIR}/doubleinputvalidator_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/intinputvalidator_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sectionproxymodel_tests.cpp
    ${CMAKE_CURRENT_LIST_DIR}/sortfilterproxymodel_tests.cpp

    ${CMAKE_CURRENT_LIST_DIR}/sectionedmodel.h
    )

set(MODULE_TEST_LINK
    Qt::Qml
    muse_uicomponents_qml
    )

include(SetupGTest)
