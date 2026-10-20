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
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */

pragma ComponentBehavior: Bound

import QtQuick

import Muse.Extensions

Item {
    id: root

    property bool active: true
    readonly property string title: prv.host ? prv.host.title : ""

    signal closeRequested
    signal initialWidthRequested(real width)

    //! Replace the current extension using a single request so its URI and action change together.
    function load(extension, action) {
        root.unload();
        if (extension.length) {
            prv.request = {
                extension: extension,
                action: action
            };
            if (prv.host) {
                prv.host.loadPlugin();
            }
        }
    }

    //! Unload synchronously before the caller changes the extension's score context.
    function unload() {
        if (prv.host) {
            prv.host.unloadPlugin();
        }
        prv.request = null;
    }

    QtObject {
        id: prv
        property var request: null
        readonly property PluginHost host: loader.item as PluginHost
    }

    Loader {
        id: loader
        anchors.fill: parent
        active: root.active && prv.request !== null

        sourceComponent: PluginHost {}
    }

    component PluginHost: Item {
        id: pluginHost
        property alias title: builder.title
        clip: true

        Component.onCompleted: loadPlugin()

        //! Load once when the host is ready, then report its requested initial width.
        function loadPlugin() {
            if (!prv.request || builder.contentItem) {
                return;
            }
            const request = prv.request;
            builder.load(request.extension, request.action, pluginHost);
            const requestedWidth = builder.contentItem ? builder.contentItem.implicitWidth : 0;
            Qt.callLater(function () {
                if (prv.request === request) {
                    root.initialWidthRequested(requestedWidth > 0 ? requestedWidth : 360);
                }
            });
        }

        //! Release the builder's content before the host is destroyed or reused.
        function unloadPlugin() {
            builder.unload();
        }

        ExtensionBuilder {
            id: builder
            onCloseRequested: {
                const request = prv.request;
                Qt.callLater(function () {
                    if (prv.request === request) {
                        root.closeRequested();
                    }
                });
            }
        }

        Binding {
            target: builder.contentItem
            property: "width"
            value: pluginHost.width
            when: Boolean(builder.contentItem)
            restoreMode: Binding.RestoreNone
        }

        Binding {
            target: builder.contentItem
            property: "height"
            value: pluginHost.height
            when: Boolean(builder.contentItem)
            restoreMode: Binding.RestoreNone
        }
    }
}
