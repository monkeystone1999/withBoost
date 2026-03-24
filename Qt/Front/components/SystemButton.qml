import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import AnoMap.Front

AbstractButton {
    id: rootItem
    width: 46
    height: 48
    hoverEnabled: true

    property int duration: 400
    property url iconSource
    property color bgColor: Theme.bgPrimary
    property color hoverIconChange: Theme.iconChange
    property bool isClose: false

    leftPadding: 0
    topPadding: 0
    rightPadding: 0
    bottomPadding: 0
    leftInset: 0
    topInset: 0
    rightInset: 0
    bottomInset: 0

    Component.onCompleted: {
        console.log("SystemButton created, source:", iconSource);
    }

    background: Rectangle {
        color: {
            if (!rootItem.enabled)
                return rootItem.bgColor;
            if (rootItem.isClose && rootItem.hovered)
                return "#E81123";
            if (rootItem.hovered)
                return Theme.bgThird;
            return rootItem.bgColor;
        }
        Behavior on color {
            ColorAnimation {
                duration: rootItem.duration
            }
        }
    }

    contentItem: Item {
        Image {
            id: iconImage
            anchors.centerIn: parent
            visible: true
            layer.enabled: true
            source: rootItem.iconSource
            sourceSize.width: 33
            sourceSize.height: 33
            fillMode: Image.PreserveAspectFit
        }

        MultiEffect {
            anchors.fill: iconImage
            source: iconImage
            colorization: 1.0
            colorizationColor: rootItem.hovered ? rootItem.hoverIconChange : Theme.iconColor
            Behavior on colorizationColor {
                ColorAnimation {
                    duration: rootItem.duration
                }
            }
        }
    }
}
