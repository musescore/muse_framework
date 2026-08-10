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

ListItemBlank {
    id: root

    property string title: ""
    property bool isExpanded: true

    property real sideMargin: 0
    property real spacing: 4

    property bool drawZebra: true

    signal toggleRequested()

    height: 34

    normalColor: ui.theme.buttonColor

    navigation.accessible.role: MUAccessible.ListItem
    navigation.accessible.name: root.title + ", " + (root.isExpanded
                                                     ? qsTrc("ui", "expanded")
                                                     : qsTrc("ui", "collapsed"))

    navigation.onTriggered: {
        root.toggleRequested()
    }

    onClicked: {
        root.toggleRequested()
    }

    StyledTextLabel {
        anchors.fill: parent
        anchors.leftMargin: root.sideMargin
        anchors.rightMargin: root.sideMargin

        horizontalAlignment: Text.AlignLeft
        font: ui.theme.bodyBoldFont

        text: root.title
    }

    SeparatorLine {
        anchors.bottom: root.bottom

        visible: !root.drawZebra
    }
}
