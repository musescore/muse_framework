# SPDX-License-Identifier: GPL-3.0-only
# MuseScore/Audacity CLA applies
#
# Copyright (C) MuseScore/Audacity and others
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

muse_create_qml_module(muse_rcommand_qml ALIAS muse::rcommand_qml FOR muse_rcommand)

qt_add_qml_module(muse_rcommand_qml
    URI Muse.RCommand
    VERSION 1.0
    SOURCES
        commandlistmodel.cpp
        commandlistmodel.h
    QML_FILES
        CommandListDialog.qml
        CommandListPanel.qml
    IMPORTS
        TARGET muse_ui_qml
        TARGET muse_uicomponents_qml
)

fixup_qml_module_dependencies(muse_rcommand_qml)
