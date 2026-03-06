import QtQuick
import QtQuick.Controls.Basic
import AnoMap.front

TextField {
    id: root
    property string placeholder
    property real rowLength: 200
    property real heightLength: 40
    property bool isError: false
    property string errorText: ""
    property alias text: root.text
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
        implicitWidth: root.rowLength
        implicitHeight: root.heightLength
        radius: 15
        color: root.bg
        border.color: root.isError ? "red" : "transparent"
        border.width: root.isError ? 1 : 0
    }

    Text {
        leftPadding: 15
        text: root.placeholder
        // Center vertically when not focused, top when focused
        y: root.activeFocus || root.text.length > 0 ? -heightLength / 2 : (root.height - height) / 2
        font.pixelSize: root.activeFocus || root.text.length > 0 ? 15 : 20
        color: Theme.fontColor // Adding color for visibility

        Behavior on y {
            NumberAnimation {
                duration: 200
                easing.type: Easing.OutQuad
            }
        }
    }

    Text {
        visible: root.isError
        text: root.errorText
        color: "red"
        font.pixelSize: 12
        anchors.top: parent.bottom
        anchors.topMargin: 4
        anchors.left: parent.left
    }
}
