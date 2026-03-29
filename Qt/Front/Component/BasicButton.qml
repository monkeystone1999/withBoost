import QtQuick
import QtQuick.Controls
import AnoMap.Front

AbstractButton {
    id: rootItem
    hoverEnabled: true

    topPadding: 3
    bottomPadding: 3
    leftPadding: 5
    rightPadding: 5
    contentItem: Text {
        text: rootItem.text
        color: Theme.fontColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
    }
    background: Rectangle {
        radius: 10
        color: rootItem.hovered ? Theme.hanwhaSecond : Theme.bgSecondary
        Behavior on color {
            ColorAnimation {
                duration: 200
            }
        }
    }
}
