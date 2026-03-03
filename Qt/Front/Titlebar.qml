import QtQuick
import QWindowKit
import AnoMap.front

Item {
    id: root
    height: 52
    required property var window
    // QWindowKit setSystemButton 에 쓸 수 있도록 외부에 노출
    property alias minimizeBtn: minimizeButton
    property alias maximizeBtn: maximizeButton
    property alias closeBtn: closeButton
    property alias menuBtn: menuButton
    property alias alarmBtn: alarm
    Rectangle {
        anchors.fill: parent
        color: Theme.bgPrimary
        //좌측 로고
        Row {
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: 0
            spacing: 6
            Image {
                id: logoImage
                anchors.verticalCenter: parent.verticalCenter
                source: Icon.logo
                height: 36
                width: height * (85 / 77)   // SVG viewBox: 85x77 비율 유지
                sourceSize.width: width
                sourceSize.height: height
                fillMode: Image.PreserveAspectFit
            }
            // 햄버거 ≡ 메뉴 버튼
            Rectangle {
                id: menuButton
                width: 46
                height: 48
                color: menuHover.containsMouse
                       ? Theme.bgThird
                       : Theme.bgPrimary
                radius: 4
                anchors.verticalCenter: parent.verticalCenter

                Behavior on color {
                    ColorAnimation { duration: 200 }
                }

                Text {
                    anchors.centerIn: parent
                    text: "≡"
                    font.pixelSize: 24
                    color: menuHover.containsMouse ? Theme.iconChange : Theme.iconColor
                    Behavior on color { ColorAnimation { duration: 200 } }
                }

                MouseArea {
                    id: menuHover
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: menuButton.clicked()
                }

                signal clicked()
            }
        }
        Text {
            anchors.centerIn: parent
            text: "AnoMAP"
            font.family: Typography.fontFamilyPrimary
            font.pixelSize: 32
            color: Theme.fontColor
        }
        // 우측 버튼들
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
                onClicked: root.window.showMinimized()
            }
            SystemButton {
                id: maximizeButton
                iconSource: Icon.maximize
                onClicked: {
                    if (root.window.visibility === Window.Maximized || root.window.visibility === Window.FullScreen) {
                        root.window.showNormal();
                    } else {
                        root.window.showMaximized();
                    }
                }
            }
            SystemButton {
                id: closeButton
                iconSource: Icon.close
                isClose: true
                onClicked: root.window.close()
            }
        }
    }
}
