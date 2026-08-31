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

import "internal"

StyledDialogView {
    id: root

    property string appName: ""
    property string version: ""
    property bool readyToInstall: false
    property alias notes: view.notes
    property alias previousReleasesNotes: view.previousReleasesNotes

    contentWidth: 644
    contentHeight: 474

    margins: 22

    onNavigationActivateRequested: {
        buttons.focusOnFirst()
    }

    onAccessibilityActivateRequested: {
        accessibleInfo.readInfo()
    }

    ColumnLayout {
        id: content

        anchors.fill: parent
        spacing: 24

        AccessibleItem {
             id: accessibleInfo

             visualItem: content
             role: MUAccessible.Button
             name: releaseTitleLabel.text + " " + view.notes + " " + buttons.defaultButtonName

             function readInfo() {
                 accessibleInfo.ignored = false
                 accessibleInfo.focused = true
             }

             function resetFocus() {
                 accessibleInfo.ignored = true
                 accessibleInfo.focused = false
             }
         }

        Column {
            Layout.alignment: Qt.AlignTop

            spacing: 8

            StyledTextLabel {
                id: releaseTitleLabel

                text: root.readyToInstall
                      ? qsTrc("update", "A new update is ready to install")
                      : qsTrc("update", "A new version of %1 is available!").arg(root.appName)
                font: ui.theme.headerBoldFont
            }

            StyledTextLabel {
                id: releaseDescriptionLabel

                width: content.width

                visible: root.readyToInstall

                text: qsTrc("update", "%1 has downloaded an update and is ready to install. "
                                      + "A restart will be required to complete the installation. "
                                      + "If you have any unsaved changes, you will be prompted to save them first.")
                      .arg(root.appName)
                horizontalAlignment: Qt.AlignLeft
                wrapMode: Text.WordWrap
            }

            StyledTextLabel {
                id: releaseNotesLabel

                visible: !root.readyToInstall

                text: qsTrc("update", "Release notes")
                font: ui.theme.largeBodyBoldFont
                horizontalAlignment: Qt.AlignLeft
            }
        }

        SeparatorLine {
            Layout.leftMargin: -root.margins
            Layout.rightMargin: -root.margins
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true

            spacing: 12

            StyledTextLabel {
                visible: root.readyToInstall

                text: root.version.length > 0
                      ? qsTrc("update", "%1 Release notes").arg(root.version)
                      : qsTrc("update", "Release notes")
                font: ui.theme.largeBodyBoldFont
                horizontalAlignment: Qt.AlignLeft
            }

            ReleaseNotesView {
                id: view

                Layout.fillWidth: true
                Layout.fillHeight: true
            }
        }

        SeparatorLine {
            Layout.leftMargin: -root.margins
            Layout.rightMargin: -root.margins
        }

        AppReleaseInfoBottomPanel {
            id: buttons

            Layout.fillWidth: true
            Layout.preferredHeight: childrenRect.height
            Layout.alignment: Qt.AlignBottom

            navigationPanel.section: root.navigationSection
            navigationPanel.order: 1

            onRemindLaterRequested: {
                root.ret = { errcode: 0, value: "remindLater" }
                root.hide()
            }

            onInstallRequested: {
                root.ret = { errcode: 0, value: "install" }
                root.hide()
            }

            onSkipRequested: {
                root.ret = { errcode: 0, value: "skip" }
                root.hide()
            }
        }
    }
}
