/*
 * SPDX-License-Identifier: GPL-3.0-only
 * MuseScore/Audacity CLA applies
 *
 * Copyright (C) MuseScore/Audacity and others
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
import Muse.RCommand

Rectangle {

    id: root

    objectName: "DiagnosticCommandsPanel"
    color: ui.theme.backgroundPrimaryColor

    CommandListModel {
        id: commandsModel
    }

    Item {
        id: toolPanel
        anchors.left: parent.left
        anchors.right: parent.right
        height: 48

        TextInputField {
            id: inputItem
            anchors.left: parent.left
            anchors.right: btnRow.left
            anchors.verticalCenter: parent.verticalCenter
            anchors.margins: 16
            clearTextButtonVisible: true
            onTextChanged: function(newTextValue) {
                commandsModel.find(newTextValue)
            }
        }

        Row {
            id: btnRow
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.rightMargin: 16
            width: childrenRect.width
            spacing: 8

            FlatButton {
                anchors.verticalCenter: parent.verticalCenter
                text: "Print"
                onClicked: commandsModel.print()
            }
        }
    }

    ListView {
        anchors.top: toolPanel.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        clip: true
        model: commandsModel
        // section.property: "groupRole"
        // section.delegate: Rectangle {
        //     width: parent.width
        //     height: 24
        //     color: ui.theme.backgroundSecondaryColor
        //     StyledTextLabel {
        //         anchors.fill: parent
        //         anchors.margins: 2
        //         horizontalAlignment: Qt.AlignLeft
        //         text: section
        //     }
        // }
        delegate: ListItemBlank {

            anchors.left: parent ? parent.left : undefined
            anchors.right: parent ? parent.right : undefined
            height: 64

            StyledTextLabel {
                anchors.fill: parent
                anchors.leftMargin: 16
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignLeft
                font.family: "Consolas"
                text: itemData
            }

            onClicked: function() {
                commandsModel.dispatch(model.index)
            }
        }
    }
}