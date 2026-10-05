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
    property alias readyToInstall: buttons.readyToInstall
    property alias notes: view.notes
    property alias previousReleasesNotes: view.previousReleasesNotes
    property alias autoUpdateEnabled: autoUpdateSetting.checked

    contentWidth: 644
    contentHeight: 474

    margins: 22

    property bool isFinished: false

    function setResult(action) {
        root.isFinished = true
        root.ret = {
            errcode: 0,
            value: {
                action: action,
                autoUpdateEnabled: root.autoUpdateEnabled
            }
        }
    }

    function finish(action) {
        root.setResult(action)
        root.hide()
    }

    onAboutToClose: {
        if (!root.isFinished) {
            root.setResult("close")
        }
    }

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

        StyledTextLabel {
            id: releaseTitleLabel

            text: qsTrc("update", "%1 %2 is available!").arg(root.appName).arg(root.version)
            font: ui.theme.headerBoldFont
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
                text: qsTrc("update", "Release notes")
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

        AutoUpdateSetting {
            id: autoUpdateSetting

            Layout.fillWidth: true

            appName: root.appName

            navigationPanel.section: root.navigationSection
            navigationPanel.order: 2
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

            isRemindMeLaterButtonEnabled: !root.autoUpdateEnabled

            navigationPanel.section: root.navigationSection
            navigationPanel.order: 1

            onRemindLaterRequested: {
                root.finish("remindLater")
            }

            onInstallRequested: {
                root.finish("install")
            }

            onSkipRequested: {
                root.finish("skip")
            }
        }
    }
}
