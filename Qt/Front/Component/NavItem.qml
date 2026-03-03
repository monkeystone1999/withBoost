import QtQuick
import QtQuick.Controls
import AnoMap.front

// 사이드 패널의 개별 메뉴 항목
Rectangle {
    id: root

    property string label: ""
    property string iconChar: ""
    property bool isActive: false

    signal clicked()

    height: 48
    color: isActive
           ? (Theme.isDark ? "#3A3A3C" : "#E5E5EA")
           : (hoverArea.containsMouse ? (Theme.isDark ? "#2C2C2E" : "#F2F2F7") : "transparent")

    Behavior on color {
        ColorAnimation { duration: 150 }
    }

    // 활성 항목 왼쪽 액센트 라인
    Rectangle {
        width: 3
        height: parent.height * 0.6
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        color: Theme.hanwhaFirst
        radius: 2
        opacity: root.isActive ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 150 } }
    }

    Row {
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 20
        spacing: 14

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.iconChar
            font.pixelSize: 16
            color: root.isActive ? Theme.hanwhaFirst : Theme.fontColor
            Behavior on color { ColorAnimation { duration: 150 } }
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.label
            font.family: Typography.fontFamilySecondary
            font.pixelSize: Typography.thirdFont
            font.bold: root.isActive
            color: root.isActive ? Theme.hanwhaFirst : Theme.fontColor
            Behavior on color { ColorAnimation { duration: 150 } }
        }
    }

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
