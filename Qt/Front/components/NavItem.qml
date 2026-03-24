import QtQuick
import QtQuick.Controls
import Qt5Compat.GraphicalEffects
import AnoMap.Front

Rectangle {
    id: rootItem

    property string label: ""
    property url iconSource: ""
    property bool isActive: false

    signal clicked

    height: 48
    color: isActive ? (Theme.isDark ? "#3A3A3C" : "#E5E5EA") : (hoverArea.containsMouse ? (Theme.isDark ? "#2C2C2E" : "#F2F2F7") : "transparent")

    Behavior on color {
        ColorAnimation {
            duration: 150
        }
    }

    Rectangle {
        width: 3
        height: parent.height * 0.6
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        color: Theme.hanwhaFirst
        radius: 2
        opacity: rootItem.isActive ? 1.0 : 0.0
        Behavior on opacity {
            NumberAnimation {
                duration: 150
            }
        }
    }

    Row {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 20
        spacing: 14

        Item {
            anchors.verticalCenter: parent.verticalCenter
            width: 20
            height: 20

            Image {
                id: iconImg
                anchors.fill: parent
                source: rootItem.iconSource
                sourceSize.width: 20
                sourceSize.height: 20
                fillMode: Image.PreserveAspectFit
                visible: false
            }

            ColorOverlay {
                anchors.fill: iconImg
                source: iconImg
                color: rootItem.isActive ? Theme.hanwhaFirst : Theme.fontColor
                Behavior on color {
                    ColorAnimation {
                        duration: 150
                    }
                }
            }
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: rootItem.label
            font.family: Typography.fontFamilySecondary
            font.pixelSize: Typography.thirdFont
            font.bold: rootItem.isActive
            color: rootItem.isActive ? Theme.hanwhaFirst : Theme.fontColor
            Behavior on color {
                ColorAnimation {
                    duration: 150
                }
            }
        }
    }

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: rootItem.clicked()
    }
}
