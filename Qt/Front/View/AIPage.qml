import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front

Item {
    id: root

    // Page Name Property
    property string pageName: "AI"
    signal requestPage(string pageName)

    Rectangle {
        anchors.fill: parent
        color: "transparent"

        Text {
            anchors.centerIn: parent
            text: qsTr("AI Page")
            color: Theme.fontColor
            font.family: Typography.fontFamilyPrimary
            font.pixelSize: 24
        }
    }
}
