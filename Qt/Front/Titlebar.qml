import QtQuick
import QWindowKit
import AnoMap.Front

Item {
    id: rootItem
    height: 52
    required property var window
    property alias minimizeButtonItem: minimizeButton // system buttons
    property alias maximizeButtonItem: maximizeButton
    property alias closeButtonItem: closeButton
    property alias menuButtonItem: menuButton
    property alias alarmButtonItem: alarm
    Rectangle {
        anchors.fill: parent
        color: Theme.bgPrimary
        Row {
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: 0
            spacing: 6
            Image {
                id: logoImage
                readonly property int logoH: 36
                readonly property int logoW: logoH * (85 / 77)
                anchors.verticalCenter: parent.verticalCenter
                source: Icon.logo
                height: logoH
                width: logoW
                sourceSize.width: logoW
                sourceSize.height: logoH
                fillMode: Image.PreserveAspectFit

                MouseArea {
                    id: logoHoverArea
                    anchors.fill: parent
                    hoverEnabled: true
                }
            }
            Rectangle {
                id: menuButton
                width: 46
                height: 48
                color: menuHoverArea.containsMouse ? Theme.bgThird : Theme.bgPrimary
                radius: 4
                anchors.verticalCenter: parent.verticalCenter

                Behavior on color {
                    ColorAnimation {
                        duration: 200
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: "\u2630"
                    font.pixelSize: 20
                    color: menuHoverArea.containsMouse ? Theme.iconChange : Theme.iconColor
                    Behavior on color {
                        ColorAnimation {
                            duration: 200
                        }
                    }
                }

                MouseArea {
                    id: menuHoverArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: menuButton.clicked()
                }

                signal clicked
            }
        }
        Text {
            anchors.centerIn: parent
            text: "AnoMAP"
            font.family: Typography.fontFamilyPrimary
            font.pixelSize: 32
            color: Theme.fontColor
        }
        // source: "image://svg/Core/Logos/OnlyLogo.svg?color=" + Theme.iconChange
        Row {
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            SystemButton {
                id: alarm
                iconSource: Icon.alarm
            }
            SystemButton {
                id: minimizeButton
                iconSource: Icon.minimize
                onClicked: rootItem.window.showMinimized()
            }
            SystemButton {
                id: maximizeButton
                iconSource: Icon.maximize
                onClicked: {
                    if (rootItem.window.visibility === Window.Maximized || rootItem.window.visibility === Window.FullScreen) {
                        rootItem.window.showNormal();
                    } else {
                        rootItem.window.showMaximized();
                    }
                }
            }
            SystemButton {
                id: closeButton
                iconSource: Icon.close
                isClose: true
                onClicked: rootItem.window.close()
            }
        }
    }

    Rectangle {
        id: logoTooltip
        visible: logoHoverArea.containsMouse
        width: tooltipText.width + 20
        height: tooltipText.height + 10
        color: Theme.bgSecondary
        border.color: Theme.hanwhaFirst
        border.width: 1
        radius: 4
        z: 1000

        x: 12
        y: rootItem.height

        Text {
            id: tooltipText
            anchors.centerIn: parent
            text: "it's made from Veda 5th \"Team 4\" the AnoMap,\nright's come from AnoMap Team and this logo's right from hanwha\ngithub : https://github.com/veda-Anomap\ndetail : https://github.com/veda-Anomap/QtClient\nhanwha : https://profile.hanwhavision.com/kr/corporate-identity.html"
            color: Theme.fontColor
            font.pixelSize: 12
            horizontalAlignment: Text.AlignLeft
        }
    }
}
