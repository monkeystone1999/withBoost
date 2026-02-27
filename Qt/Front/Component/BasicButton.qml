import QtQuick
import QtQuick.Controls
import AnoMap.front

Button {
    id: root
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
