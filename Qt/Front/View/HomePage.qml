import QtQuick
import QtQuick.Controls
import AnoMap.front

Item {
    id: root

    property string pageName: "Home"
    signal requestPage(string pageName)

    Rectangle {
        anchors.fill: parent
        color: Theme.bgPrimary

        Text {
            anchors.centerIn: parent
            text: "Home"
            color: Theme.fontColor
            font.pixelSize: 24
        }
    }
}
