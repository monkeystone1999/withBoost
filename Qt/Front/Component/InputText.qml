import QtQuick
import QtQuick.Controls.Basic
import AnoMap.Front

TextField {
    id: rootItem
    property string placeholder
    property real rowLength: 200
    property real heightLength: 40
    property bool isError: false
    property string errorText: ""
    property alias text: rootItem.text
    maximumLength: -1 // Explicitly allow unlimited characters to override potential IME limits
    font.pixelSize: 17
    // Layouts require implicit sizes or Layout attached properties
    implicitWidth: rowLength
    implicitHeight: heightLength
    verticalAlignment: TextInput.AlignVCenter
    leftPadding: 15
    readonly property color bg: Theme.bgSecondary
    color: Theme.fontColor
    background: Rectangle {
        implicitWidth: rootItem.rowLength
        implicitHeight: rootItem.heightLength
        radius: 15
        color: rootItem.bg
        border.color: rootItem.isError ? "red" : "transparent"
        border.width: rootItem.isError ? 1 : 0
    }

    Text {
        leftPadding: 15
        text: rootItem.placeholder
        // Center vertically when not focused, top when focused
        y: rootItem.activeFocus || rootItem.text.length > 0 ? -heightLength / 2 : (rootItem.height - height) / 2
        font.pixelSize: rootItem.activeFocus || rootItem.text.length > 0 ? 15 : 20
        color: Theme.fontColor // Adding color for visibility

        Behavior on y {
            NumberAnimation {
                duration: 200
                easing.type: Easing.OutQuad
            }
        }
    }

    Text {
        visible: rootItem.isError
        text: rootItem.errorText
        color: "red"
        font.pixelSize: 12
        anchors.top: parent.bottom
        anchors.topMargin: 4
        anchors.left: parent.left
    }
}
