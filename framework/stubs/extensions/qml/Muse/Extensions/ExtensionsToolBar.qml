import QtQuick

import Muse.Ui
import Muse.UiComponents

StyledToolBarView {
    id: stub

    property bool isEmpty: true

    visible: false

    StyledTextLabel {
        anchors.centerIn: parent
        text: "ExtensionsToolBar Stub"
    }
}
