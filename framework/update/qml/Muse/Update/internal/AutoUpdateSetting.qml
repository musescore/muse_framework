/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2026 MuseScore Limited and others
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
import QtQuick
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

ColumnLayout {
    id: root

    property string appName: ""
    property alias checked: toggle.checked

    property NavigationPanel navigationPanel: NavigationPanel {
        name: "AutoUpdatePanel"
        direction: NavigationPanel.Vertical
    }

    spacing: 8

    ToggleButton {
        id: toggle

        Layout.fillWidth: true

        text: qsTrc("update", "Download and install future %1 updates automatically").arg(root.appName)

        navigation.name: "AutoUpdateToggle"
        navigation.panel: root.navigationPanel
        navigation.row: 0

        onToggled: {
            checked = !checked
        }
    }

    StyledTextLabel {
        Layout.fillWidth: true

        text: qsTrc("update", "You can change this anytime in <b>Preferences > General</b>.")

        horizontalAlignment: Qt.AlignLeft
        wrapMode: Text.WordWrap
        opacity: 0.7
    }
}
