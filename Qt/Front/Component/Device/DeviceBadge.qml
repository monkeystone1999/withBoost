import QtQuick

Rectangle {
    id: rootItem
    property string labelText: ""
    property bool isActive: false

    visible: isActive
    width: badgeText.implicitWidth + 10
    height: 18
    radius: 4
    color: "#2a2a4a"

    Text {
        id: badgeText
        anchors.centerIn: parent
        text: rootItem.labelText
        color: "#8899cc"
        font.pixelSize: 9
    }
}
