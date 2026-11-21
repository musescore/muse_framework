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

import Muse.Ui
import Muse.UiComponents

Rectangle {
    id: root

    property bool allowAlpha: false
    property bool isIndeterminate: false

    property alias navigation: navCtrl

    signal newColorSelected(var newColor)

    implicitHeight: 30
    width: parent.width

    opacity: enabled ? 1 : ui.theme.itemOpacityDisabled

    radius: 3
    color: "#000000"

    NavigationFocusBorder { navigationCtrl: navCtrl }

    border.width: Math.max(ui.theme.borderWidth, 1)
    border.color: ui.theme.strokeColor

    ColorPickerModel {
        id: colorPickerModel

        onColorSelected: function(color) {
            root.newColorSelected(color)
        }
    }

    QtObject {
        id: prv

        function selectColor() {
            colorPickerModel.selectColor(root.color, root.allowAlpha)
        }
    }

    NavigationControl {
        id: navCtrl
        name: root.objectName != "" ? root.objectName : "ColorPicker"
        enabled: root.enabled && root.visible
        accessible.role: MUAccessible.Button
        accessible.name: Utils.accessibleColorDescription(root.color)

        onTriggered: {
            prv.selectColor()
        }
    }

    Item {
        anchors.fill: parent
        // keep the border visible:
        anchors.margins: root.border.width
        visible: root.allowAlpha && root.color.a < 1

        Canvas {
            anchors.fill: parent

            property int squareSize: 5

            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()

            onPaint: {
                var ctx = getContext("2d")

                ctx.fillStyle = "#ffffff"
                ctx.fillRect(0, 0, width, height)
                ctx.fillStyle = "#cccccc"

                for (var y = 0; y < height; y += squareSize)
                    for (var x = ((y / squareSize) % 2) * squareSize; x < width; x += 2 * squareSize)
                        ctx.fillRect(x, y, squareSize, squareSize)
            }
        }

        // the color on top of the checkerboard
        Rectangle { anchors.fill: parent; color: root.color }
    }

    StyledIconLabel {
        anchors.fill: parent
        iconCode: IconCode.QUESTION_MARK
        visible: root.isIndeterminate
    }

    MouseArea {
        id: clickableArea
        anchors.fill: parent

        enabled: root.enabled
        hoverEnabled: true

        onClicked: {
            navCtrl.requestActiveByInteraction()

            prv.selectColor()
        }
    }

    states: [
        State {
            name: "HOVERED"
            when: clickableArea.containsMouse && !clickableArea.pressed

            PropertyChanges { target: root; border.color: ui.theme.accentColor }
        },

        State {
            name: "PRESSED"
            when: clickableArea.pressed

            PropertyChanges { target: root; border.color: ui.theme.fontPrimaryColor }
        }
    ]
}
