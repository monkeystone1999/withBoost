import QtQuick
import QtQuick.Controls
import AnoMap.front

AbstractButton {
    id: root
    hoverEnabled: true

    topPadding: 3
    bottomPadding: 3
    leftPadding: 5
    rightPadding: 5
    contentItem: Text {
        text: root.text
        color: Theme.fontColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: 10
        color: root.hovered ? Theme.hanwhaSecond : Theme.bgSecondary
        Behavior on color {
            ColorAnimation {
                duration: 200
            }
        }
    }
}
