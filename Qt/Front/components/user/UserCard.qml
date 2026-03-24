import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.Front

Rectangle {
    id: rootItem

    property string userId: ""
    property string username: ""
    property string email: ""
    property string role: "user"
    property bool isOnline: false
    property string lastLogin: ""
    property string ipAddress: ""
    property int activeCameras: 0

    signal tapped
    signal rightTapped(real x, real y)

    width: 320
    height: 140
    color: Theme.bgSecondary
    radius: 10
    border.color: isOnline ? "#4caf50" : "#888888"
    border.width: 2

    property bool isHovered: false

    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onEntered: rootItem.isHovered = true
        onExited: rootItem.isHovered = false

        onClicked: mouse => {
            if (mouse.button === Qt.LeftButton) {
                rootItem.tapped();
            } else if (mouse.button === Qt.RightButton) {
                rootItem.rightTapped(mouse.x, mouse.y);
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: Theme.hanwhaFirst
        opacity: rootItem.isHovered ? 0.1 : 0
        Behavior on opacity {
            NumberAnimation {
                duration: 150
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Text {
                text: rootItem.username
                color: Theme.fontColor
                font.pixelSize: 18
                font.bold: true
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            // Role Badge
            Rectangle {
                visible: rootItem.role === "admin"
                width: 60
                height: 24
                radius: 12
                color: "#ff9800"

                Text {
                    anchors.centerIn: parent
                    text: "ADMIN"
                    color: "white"
                    font.pixelSize: 11
                    font.bold: true
                }
            }

            // Online Status Indicator
            Rectangle {
                width: 12
                height: 12
                radius: 6
                color: rootItem.isOnline ? "#4caf50" : "#888888"
            }
        }

        // Email
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "\uf0e0"
                font.pixelSize: 14
                color: Theme.hanwhaFirst
            }

            Text {
                text: rootItem.email
                color: Theme.isDark ? "#aaaaaa" : "#666666"
                font.pixelSize: 13
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
        }

        // IP Address
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            visible: rootItem.ipAddress !== ""

            Text {
                text: "\uf124"
                font.pixelSize: 14
                color: Theme.hanwhaFirst
            }

            Text {
                text: rootItem.ipAddress
                color: Theme.isDark ? "#aaaaaa" : "#666666"
                font.pixelSize: 13
            }
        }

        // Active Cameras Count
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            Text {
                text: "\uf03d"
                font.pixelSize: 14
                color: Theme.hanwhaFirst
            }

            Text {
                text: qsTr("Active Cameras: %1").arg(rootItem.activeCameras)
                color: Theme.isDark ? "#aaaaaa" : "#666666"
                font.pixelSize: 13
            }
        }

        // Last Login
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            visible: rootItem.lastLogin !== ""

            Text {
                text: "\uf017"
                font.pixelSize: 14
                color: Theme.hanwhaFirst
            }

            Text {
                text: qsTr("Last Login: %1").arg(rootItem.lastLogin)
                color: Theme.isDark ? "#888888" : "#999999"
                font.pixelSize: 11
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
        }
    }
}
