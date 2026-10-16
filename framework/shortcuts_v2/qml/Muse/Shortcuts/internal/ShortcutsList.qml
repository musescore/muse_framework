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

ValueList {
    id: root

    keyRoleName: "title"
    keyTitle: qsTrc("shortcuts", "action")
    valueRoleName: "sequence"
    valueTitle: qsTrc("shortcuts", "shortcut")
    iconRoleName: "icon"
    iconColorRoleName: "iconColor"
    sectionRoleName: "group"
    readOnly: true

    property var sourceModel: null
    property string searchText: ""

    readonly property var sourceSelection: filterModel.mapSelectionToSource(root.selection)

    signal startEditCurrentShortcutRequested()

    QtObject {
        id: prv

        property var collapsedSectionsBeforeSearch: null
    }

    onSearchTextChanged: {
        if (Boolean(root.searchText)) {
            if (prv.collapsedSectionsBeforeSearch === null) {
                prv.collapsedSectionsBeforeSearch = root.collapsedSections
                root.expandAllSections()
            }
        } else if (prv.collapsedSectionsBeforeSearch !== null) {
            root.collapsedSections = prv.collapsedSectionsBeforeSearch
            prv.collapsedSectionsBeforeSearch = null
        }
    }

    model: SortFilterProxyModel {
        id: filterModel
        sourceModel: root.sourceModel

        filters: [
            FilterValue {
                roleName: "title"
                roleValue: ""
                compareType: CompareType.NotEqual
            },
            FuzzyFilter {
                id: fuzzyFilter

                enabled: Boolean(fuzzyPattern)
                fuzzyPattern: root.searchText
                roleName: "searchKey"
            }
        ]
        sorters: [
            FuzzyScoreSorter {
                fuzzyFilter: fuzzyFilter
                enabled: fuzzyFilter.enabled
            }
        ]
    }

    onHandleItem: {
        root.startEditCurrentShortcutRequested()
    }
}
