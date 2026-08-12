/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore-CLA-applies
 *
 * MuseScore Studio
 * Music Composition & Notation
 *
 * Copyright (C) 2021 MuseScore Limited and others
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

    property alias canEditCurrentShortcut: editButton.enabled
    property alias canClearCurrentShortcuts: clearButton.enabled

    property alias searchText: searchField.searchText

    property int buttonMinWidth: 0

    property var presets: null
    property string currentPresetName: ""
    property bool isCurrentPresetEdited: false
    property bool canDeleteCurrentPreset: false

    signal startEditCurrentShortcutRequested()
    signal clearSelectedShortcutsRequested()
    signal presetChangeRequested(string presetName)
    signal resetPresetRequested()
    signal deletePresetRequested()

    spacing: 12

    property NavigationPanel navigation: NavigationPanel {
        name: "ShortcutsTopPanel"
        enabled: root.enabled && root.visible
        accessible.name: qsTrc("shortcuts", "Shortcuts top panel")

        onActiveChanged: function(active) {
            if (active) {
                root.forceActiveFocus()
            }
        }
    }

    function setSearchText(text) {
        searchField.currentText = text
    }

    RowLayout {
        Layout.fillWidth: true

        StyledTextLabel {
            text: qsTrc("shortcuts", "Presets:")
        }

        StyledDropdown {
            id: presetsDropdown

            Layout.fillWidth: true

            model: root.presets
            textRole: "title"
            valueRole: "name"

            currentIndex: presetsDropdown.indexOfValue(root.currentPresetName)

            navigation.name: "ShortcutsPresetDropdown"
            navigation.panel: root.navigation
            navigation.order: 1

            onActivated: function(index, value) {
                root.presetChangeRequested(value)
            }
        }

        FlatButton {
            icon: IconCode.UNDO
            toolTipTitle: qsTrc("shortcuts", "Reset preset")

            enabled: root.isCurrentPresetEdited

            navigation.name: "ResetPresetButton"
            navigation.panel: root.navigation
            navigation.order: 2

            onClicked: {
                root.resetPresetRequested()
            }
        }

        // NOTE: enable after adding user's presets
        // FlatButton {
        //     icon: IconCode.DELETE_TANK
        //     toolTipTitle: qsTrc("shortcuts", "Delete preset")

        //     enabled: root.canDeleteCurrentPreset

        //     navigation.name: "DeletePresetButton"
        //     navigation.panel: root.navigation
        //     navigation.order: 3

        //     onClicked: {
        //         root.deletePresetRequested()
        //     }
        // }
    }

    RowLayout {
        Layout.fillWidth: true

        FlatButton {
            id: editButton

            minWidth: root.buttonMinWidth

            text: qsTrc("shortcuts", "Define…")

            navigation.name: "DefineShortcutButton"
            navigation.panel: root.navigation
            navigation.order: 4

            onClicked: {
                root.startEditCurrentShortcutRequested()
            }
        }

        FlatButton {
            id: clearButton

            minWidth: root.buttonMinWidth

            text: qsTrc("global", "Clear")

            navigation.name: "ClearShortcutsButton"
            navigation.panel: root.navigation
            navigation.order: 5

            onClicked: {
                root.clearSelectedShortcutsRequested()
            }
        }

        Item { Layout.fillWidth: true }

        SearchField {
            id: searchField

            Layout.preferredWidth: 160

            hint: qsTrc("shortcuts", "Search shortcut")

            navigation.name: "ShortcutSearchField"
            navigation.panel: root.navigation
            navigation.order: 6
        }
    }
}
