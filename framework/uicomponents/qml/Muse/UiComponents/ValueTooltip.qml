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

import Muse.Ui 1.0
import Muse.UiComponents

StyledPopupView {
    id: root

    property var value
    property int decimalPlaces: 1
    property string unitText: ""
    property string format: "%1"

    property string text: {
        const v = Number(root.value)
        return (isFinite(v) ? v.toFixed(root.decimalPlaces) : root.value) + root.unitText
    }
    property string sizingText: root.text

    property int showDelay: ui.theme.tooltipDelay
    property int hideDelay: ui.theme.tooltipDelay

    padding: 8
    margins: 8
    contentWidth: Math.ceil(Math.max(fontMetrics.advanceWidth(root.formatted(root.sizingText)), fontMetrics.advanceWidth(valueLabel.text)))
    contentHeight: Math.ceil(fontMetrics.height)

    placementPolicies: PopupView.PreferAbove
    openPolicies: PopupView.NoActivateFocus

    //! NOTE The tooltip never takes navigation focus, so it must not
    //! reactivate the captured control on close - that would yank the
    //! keyboard focus back while the user navigates away
    activateParentOnClose: false

    StyledTextLabel {
        id: valueLabel

        anchors.fill: parent
        text: root.formatted(root.text)
    }

    FontMetrics {
        id: fontMetrics

        font: valueLabel.font
    }

    Connections {
        target: root.parent

        function onXChanged() {
            root.repositionWindowIfNeed()
        }

        function onYChanged() {
            root.repositionWindowIfNeed()
        }
    }

    Timer {
        id: openTimer

        interval: root.showDelay
        repeat: false

        onTriggered: {
            root.open()
        }
    }

    Timer {
        id: closeTimer

        interval: root.hideDelay
        repeat: false

        onTriggered: {
            root.close()
        }
    }

    function formatted(value) {
        if (!root.format.includes("%1")) {
            console.warn("ValueTooltip: format is missing the %1 placeholder:", root.format)
            return root.format + value
        }
        return root.format.arg(value)
    }

    function show(noDelay = false) {
        if (noDelay) {
            root.open()
        } else {
            openTimer.restart()
        }
        closeTimer.stop()
    }

    function hide(noDelay = false) {
        if (noDelay) {
            root.close()
        } else {
            closeTimer.restart()
        }
        openTimer.stop()
    }
}
