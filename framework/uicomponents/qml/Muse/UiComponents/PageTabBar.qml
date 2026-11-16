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

pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

import Muse.Ui
import Muse.UiComponents

Item {
    id: root

    width: radioButtonList.width
    height: radioButtonList.height

    property PageTabsModel model: null

    property alias navigation: navPanel
    property string accessibleName: ""

    property string currentUri: ""

    signal selected(string uri)

    function select(uri) {
        root.selected(uri)
    }

    function focusOnFirst() {
        var btn = radioButtonList.itemAtIndex(0) as PageTabButton
        if (btn) {
            btn.navigation.requestActive()
        }
    }

    Component.onCompleted: {
        if (root.model) {
            root.model.load()
        }
    }

    Binding {
        target: root.model
        property: "currentUri"
        value: root.currentUri
        when: Boolean(root.model)
    }

    NavigationPanel {
        id: navPanel
        name: "PageTabBar"
        enabled: root.enabled && root.visible
        accessible.name: root.accessibleName + " " + navPanel.directionInfo
    }

    RadioButtonGroup {
        id: radioButtonList
        spacing: 0

        model: root.model

        width: Math.max(1, contentItem.childrenRect.width)
        height: Math.max(1, contentItem.childrenRect.height)

        delegate: PageTabButton {
            required property bool isTitleBold
            required property bool isChecked
            required title
            required property string uri
            required property int index

            ButtonGroup.group: radioButtonList.radioButtonGroup

            spacing: 0
            leftPadding: 12

            normalStateFont: isTitleBold ? ui.theme.largeBodyBoldFont : ui.theme.largeBodyFont

            navigation.name: title
            navigation.panel: navPanel
            navigation.order: index

            checked: isChecked

            onToggled: {
                root.selected(uri)
            }
        }
    }
}
