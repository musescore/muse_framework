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
import Qt.labs.platform as QtPlatform

QtPlatform.FileDialog {
    id: root

    property string objectId: ""
    property var ret: null

    readonly property string allFilesExtension: "*"

    signal opened()
    signal closed()

    function show() {
        //! NOTE `selectedNameFilter` is created on first access
        root.selectedNameFilter.index = 0
        root.open()
    }

    function appendSuffixIfNeeded(filePath) {
        const extensions = root.selectedNameFilter.extensions || []
        let defaultExtension = ""
        for (let i = 0; i < extensions.length; ++i) {
            const extension = extensions[i]
            if (extension === "" || extension === root.allFilesExtension) {
                continue
            }
            if (defaultExtension === "") {
                defaultExtension = extension
            }
            if (filePath.endsWith("." + extension)) {
                return filePath
            }
        }
        return defaultExtension === "" ? filePath : filePath + "." + defaultExtension
    }

    onVisibleChanged: {
        if (visible) {
            root.opened()
        }
    }

    onAccepted: {
        if (root.fileMode === FileDialog.OpenFiles) {
            const selectedUrls = root.fileUrls || []
            root.ret = { "errcode": 0, "value": selectedUrls }
        } else if (root.fileMode === FileDialog.SaveFile) {
            root.ret = { "errcode": 0, "value": root.appendSuffixIfNeeded(root.currentFile.toString()) }
        } else {
            root.ret = { "errcode": 0, "value": root.currentFile.toString() }
        }
        root.close()
        root.closed()
    }

    onRejected: {
        root.ret = { "errcode": 3 }
        root.close()
        root.closed()
    }
}
