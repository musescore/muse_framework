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
import QtQuick.Controls
import QtQuick.Layouts

import Muse.Ui
import Muse.UiComponents

import "internal"

Item {
    id: root

    property alias model: sortFilterProxyModel.sourceModel

    property bool readOnly: false
    property var isReadOnlyFunction: null

    property string keyRoleName: "key"
    //: As in a "key/value" pair: for example, the "key" could be
    //: the name of a setting and the "value" the value of that setting.
    property string keyTitle: qsTrc("ui", "Key", "key/value")
    property string valueRoleName: "value"
    property string valueTitle: qsTrc("ui", "Value")
    property string valueTypeRole: "valueType"
    property string valueEnabledRoleName: "enabled"
    property string minValueRoleName: "min"
    property string maxValueRoleName: "max"
    property string iconRoleName: "icon"
    property string iconColorRoleName: "iconColor"
    property string sectionRoleName: ""

    property alias collapsedSections: sectionProxyModel.collapsedSections

    property alias hasSelection: selectionModel.hasSelection
    readonly property var selection: sortFilterProxyModel.mapSelectionToSource(selectionModel.selection)

    property string headerColor: ui.theme.backgroundPrimaryColor
    property bool drawZebra: true

    property int keyColumnWidth: 0
    property bool isKeyEditable: false
    property int headerCapitalization: Font.AllUppercase
    property bool startEditByDoubleClick: false
    property bool sorterEnabled: true

    property NavigationSection navigationSection: null
    property int navigationOrderStart: 0

    property alias currentIndex: view.currentIndex
    readonly property int currentSourceRow: {
        if (view.currentIndex < 0) {
            return -1
        }

        var filteredRow = sectionProxyModel.sourceRowOf(view.currentIndex)
        if (filteredRow < 0) {
            return -1
        }

        return sortFilterProxyModel.mapToSource(sortFilterProxyModel.index(filteredRow, 0)).row
    }

    signal handleItem(var index, var item)
    signal keyEdited(int sourceRow, string newKey)
    signal valueEdited(int sourceRow, string newValue)

    function expandAllSections() {
        sectionProxyModel.expandAll()
    }

    function collapseAllSections() {
        sectionProxyModel.collapseAll()
    }

    QtObject {
        id: prv

        property real valueItemWidth: 126
        property real spacing: 4
        property real sideMargin: 30
        property real rowHeight: 34

        readonly property bool sectionsEnabled: root.sectionRoleName !== ""
        property string noSectionTitle: qsTrc("ui", "Other")

        function setSectionCollapsed(section, collapsed) {
            if (collapsed) {
                //! NOTE: Otherwise the selection would stay on the items that are not visible anymore
                selectionModel.clear()
            }

            sectionProxyModel.setSectionCollapsed(section, collapsed)
        }

        function toggleSorter(sorter: Sorter) {
            if (!sorter.enabled) {
                sorter.sortOrder = Qt.AscendingOrder
                sorter.enabled = true
            } else if (sorter.sortOrder === Qt.AscendingOrder) {
                sorter.sortOrder = Qt.DescendingOrder
            } else {
                sorter.enabled = false
            }

            selectionModel.clear()
        }
    }

    Rectangle {
        id: background
        anchors.fill: parent

        color: ui.theme.backgroundPrimaryColor
        border.width: 1
        border.color: ui.theme.strokeColor
    }

    SortFilterProxyModel {
        id: sortFilterProxyModel

        sectionRoleName: root.sectionRoleName

        sorters: [
            SorterValue {
                id: keySorter
                roleName: keyRoleName
            },
            SorterValue {
                id: valueSorter
                roleName: valueRoleName
            }
        ]
    }

    SectionProxyModel {
        id: sectionProxyModel

        sourceModel: sortFilterProxyModel
        sectionRoleName: root.sectionRoleName
    }

    ItemMultiSelectionModel {
        id: selectionModel

        model: sortFilterProxyModel
    }

    Rectangle {
        id: headerBackground

        anchors.fill: header

        color: root.headerColor
        border.width: 1
        border.color: ui.theme.strokeColor

        visible: !root.drawZebra
    }

    RowLayout {
        id: header

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 38

        property NavigationPanel headerNavigation: NavigationPanel {
            name: "ValueListHeaderPanel"
            section: root.navigationSection
            enabled: header.enabled && header.visible
            direction: NavigationPanel.Horizontal
            order: root.navigationOrderStart

            //: Accessibility description of the header of a value list (table)
            accessible.name: qsTrc("ui", "Value list header panel")

            onActiveChanged: function(active) {
                if (active) {
                    root.forceActiveFocus()
                }
            }
        }

        ValueListHeaderItem {
            Layout.fillHeight: true
            Layout.preferredWidth: root.keyColumnWidth != 0 ? root.keyColumnWidth + 2 * prv.sideMargin : -1
            Layout.fillWidth: root.keyColumnWidth != 0 ? false : true
            leftMargin: prv.sideMargin
            rightMargin: prv.sideMargin

            headerTitle: keyTitle
            headerCapitalization: root.headerCapitalization
            spacing: prv.spacing
            isSorterEnabled: root.sorterEnabled ? keySorter.enabled : false
            sortOrder: keySorter.sortOrder

            navigation.panel: header.headerNavigation
            navigation.column: 0

            onClicked: {
                if (!root.sorterEnabled) {
                    return
                }

                prv.toggleSorter(keySorter)
                valueSorter.enabled = false
            }
        }

        SeparatorLine {
            id: headerSeparator
        }

        ValueListHeaderItem {
            Layout.preferredWidth: root.keyColumnWidth != 0 ? -1 : prv.valueItemWidth + prv.sideMargin
            Layout.fillWidth: root.keyColumnWidth != 0 ? true : false
            Layout.fillHeight: true
            Layout.alignment: root.keyColumnWidth != 0 ? Qt.AlignLeft : Qt.AlignRight
            Layout.leftMargin: root.keyColumnWidth != 0 ? prv.sideMargin : 18
            rightMargin: prv.sideMargin

            headerTitle: valueTitle
            headerCapitalization: root.headerCapitalization
            spacing: prv.spacing
            isSorterEnabled: root.sorterEnabled ? valueSorter.enabled : false
            sortOrder: valueSorter.sortOrder

            navigation.panel: header.headerNavigation
            navigation.column: 1

            onClicked: {
                if (!root.sorterEnabled) {
                    return
                }

                prv.toggleSorter(valueSorter)
                keySorter.enabled = false
            }
        }
    }

    SeparatorLine {
        anchors.bottom: header.bottom

        visible: !root.drawZebra
    }

    StyledListView {
        id: view

        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.leftMargin: background.border.width
        anchors.right: parent.right
        anchors.rightMargin: background.border.width

        height: Math.min(contentHeight, root.height - header.height - background.border.width - 1/*separator*/)

        model: sectionProxyModel

        property NavigationPanel navigation: NavigationPanel {
            name: "ValueListPanel"
            section: root.navigationSection
            enabled: root.enabled && root.visible
            direction: NavigationPanel.Both
            order: root.navigationOrderStart + 1

            //: Accessibility description of the body of a value list (table)
            accessible.name: qsTrc("ui", "Value list panel")

            onActiveChanged: function(active) {
                if (active) {
                    root.forceActiveFocus()
                }
            }
        }

        ScrollBar.vertical: StyledScrollBar {}

        delegate: Loader {
            id: rowLoader

            width: view.width
            height: prv.rowHeight

            readonly property int rowIndex: model.index
            readonly property bool isSectionRow: prv.sectionsEnabled && model.isSection === true
            readonly property int filteredRow: sectionProxyModel.sourceRowOf(model.index)

            sourceComponent: rowLoader.isSectionRow ? sectionItemComponent : valueItemComponent

            Component {
                id: sectionItemComponent

                ValueListSectionItem {
                    title: model.sectionName === "" ? prv.noSectionTitle : model.sectionName
                    isExpanded: model.sectionExpanded
                    drawZebra: root.drawZebra

                    spacing: prv.spacing
                    sideMargin: prv.sideMargin

                    navigation.panel: view.navigation
                    navigation.row: root.isKeyEditable ? rowLoader.rowIndex * 2 : rowLoader.rowIndex
                    navigation.column: 0

                    navigation.onNavigationEvent: function(event) {
                        switch (event.type) {
                        case NavigationEvent.Up:
                            if (rowLoader.rowIndex === 0) {
                                event.accepted = true
                            }
                            break
                        case NavigationEvent.Down:
                            if (rowLoader.rowIndex === view.model.rowCount - 1) {
                                event.accepted = true
                            }
                            break
                        case NavigationEvent.Left:
                            if (model.sectionExpanded) {
                                prv.setSectionCollapsed(model.sectionName, true)
                                event.accepted = true
                            }
                            break
                        case NavigationEvent.Right:
                            if (!model.sectionExpanded) {
                                prv.setSectionCollapsed(model.sectionName, false)
                                event.accepted = true
                            }
                            break
                        }
                    }

                    onToggleRequested: {
                        view.currentIndex = rowLoader.rowIndex
                        prv.setSectionCollapsed(model.sectionName, model.sectionExpanded)
                    }

                    onFocusChanged: {
                        if (activeFocus) {
                            view.positionViewAtIndex(rowLoader.rowIndex, ListView.Contain)
                        }
                    }
                }
            }

            Component {
                id: valueItemComponent

                ValueListItem {
                    id: listItem

                    item: model

                    property var modelIndex: sortFilterProxyModel.index(rowLoader.filteredRow, 0)
                    property int sourceRow: sortFilterProxyModel.mapToSource(modelIndex).row

                    keyRoleName: root.keyRoleName
                    valueRoleName: root.valueRoleName
                    valueTypeRole: root.valueTypeRole
                    valueEnabledRoleName: root.valueEnabledRoleName
                    minValueRoleName: root.minValueRoleName
                    maxValueRoleName: root.maxValueRoleName
                    iconRoleName: root.iconRoleName
                    iconColorRoleName: root.iconColorRoleName

                    isSelected: selectionModel.hasSelection && selectionModel.isSelected(modelIndex)
                    readOnly: root.readOnly
                    keyReadOnly: root.isKeyEditable ? root.isReadOnlyFunction(rowLoader.filteredRow) : true

                    drawZebra: root.drawZebra
                    zebraIndex: prv.sectionsEnabled ? model.indexInSection : rowLoader.rowIndex
                    keyColumnWidth: root.keyColumnWidth
                    keysEditable: root.isKeyEditable
                    startEditByDoubleClick: root.startEditByDoubleClick

                    spacing: prv.spacing
                    sideMargin: prv.sideMargin
                    valueItemWidth: prv.valueItemWidth

                    navigation.panel: view.navigation
                    navigation.enabled: root.isKeyEditable ? false : enabled
                    navigation.row: rowLoader.rowIndex
                    navigation.column: 0

                    navigation.onNavigationEvent: function(event) {
                        switch (event.type) {
                        case NavigationEvent.Up:
                            if (rowLoader.rowIndex === 0) {
                                event.accepted = true
                            }
                            break
                        case NavigationEvent.Down:
                            if (rowLoader.rowIndex === view.model.rowCount - 1) {
                                event.accepted = true
                            }
                            break
                        }
                    }

                    onClicked: {
                        selectionModel.select(modelIndex)
                        view.currentIndex = rowLoader.rowIndex
                    }

                    onDoubleClicked: {
                        selectionModel.select(modelIndex)
                        view.currentIndex = rowLoader.rowIndex
                        Qt.callLater(root.handleItem, sortFilterProxyModel.mapToSource(modelIndex), item)
                    }

                    onNavigationTriggered: {
                        root.handleItem(sortFilterProxyModel.mapToSource(modelIndex), item)
                    }

                    onFocusChanged: {
                        if (activeFocus) {
                            view.positionViewAtIndex(rowLoader.rowIndex, ListView.Contain)
                        }
                    }

                    onKeyEdited: function(newKey) {
                        root.keyEdited(sourceRow, newKey)
                    }

                    onValueEdited: function(newVal) {
                        root.valueEdited(sourceRow, newVal)
                    }
                }
            }
        }
    }

    SeparatorLine {
        x: headerSeparator.x
        orientation: Qt.Vertical
    }
}
