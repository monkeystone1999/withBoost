import QtQuick
import QtQuick.Layouts
import AnoMap.Front

Rectangle {
    id: rootItem

    property int alarmId: -1
    property string alarmTitle: "알림"
    property string alarmDetail: "상세 정보"
    property int severity: 0  // 0=info, 1=warning, 2=critical

    signal dismissRequested(int id)

    readonly property color severityColor: {
        switch (severity) {
        case 2:
            return "#F44336";
        case 1:
            return "#FF9800";
        default:
            return "#2196F3";
        }
    }

    height: hoverHandler.hovered ? expandedHeight : collapsedHeight

    readonly property int collapsedHeight: 56
    readonly property int expandedHeight: 110

    Behavior on height {
        NumberAnimation {
            duration: 200
            easing.type: Easing.OutCubic
        }
    }

    width: 300
    radius: 10
    color: Theme.bgSecondary
    clip: true

    Rectangle {
        width: 4
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: 6
        anchors.bottomMargin: 6
        radius: 2
        color: rootItem.severityColor
    }

    HoverHandler {
        id: hoverHandler
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 10
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        spacing: 4

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Rectangle {
                width: 8
                height: 8
                radius: 4
                color: rootItem.severityColor
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: rootItem.alarmTitle
                font.family: Typography.fontFamilySecondary
                font.pixelSize: Typography.teriaryFont
                font.bold: true
                color: Theme.fontColor
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            Rectangle {
                id: closeButtonItem
                width: 20
                height: 20
                radius: 10
                color: closeButtonHoverArea.containsMouse ? "#F44336" : "transparent"
                opacity: hoverHandler.hovered ? 1.0 : 0.0

                Behavior on opacity {
                    NumberAnimation {
                        duration: 150
                    }
                }
                Behavior on color {
                    ColorAnimation {
                        duration: 150
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: "x"
                    font.pixelSize: 11
                    color: closeButtonHoverArea.containsMouse ? "white" : Theme.fontColor
                    Behavior on color {
                        ColorAnimation {
                            duration: 150
                        }
                    }
                }

                MouseArea {
                    id: closeButtonHoverArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: rootItem.dismissRequested(rootItem.alarmId)
                }
            }
        }

        Text {
            text: rootItem.alarmDetail
            font.family: Typography.fontFamilySecondary
            font.pixelSize: Typography.teriaryFont - 1
            color: Theme.fontColor
            opacity: hoverHandler.hovered ? 0.75 : 0.0
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            maximumLineCount: 3
            elide: Text.ElideRight

            Behavior on opacity {
                NumberAnimation {
                    duration: 180
                }
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: "transparent"
        border.color: Theme.isDark ? "#3A3A3C" : "#D1D1D6"
        border.width: 1
    }
}
