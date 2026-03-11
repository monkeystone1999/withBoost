import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front

// ── UserCard ────────────────────────────────────────────────────────────────
// 사용자 정보를 표시하는 카드 컴포넌트
// ───────────────────────────────────────────────────────────────────────────
Rectangle {
    id: root

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

    // 호버 효과
    property bool isHovered: false
    
    MouseArea {
        id: hoverArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        
        onEntered: root.isHovered = true
        onExited: root.isHovered = false
        
        onClicked: mouse => {
            if (mouse.button === Qt.LeftButton) {
                root.tapped();
            } else if (mouse.button === Qt.RightButton) {
                root.rightTapped(mouse.x, mouse.y);
            }
        }
    }

    // 호버 시 배경 효과
    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: Theme.hanwhaFirst
        opacity: root.isHovered ? 0.1 : 0
        Behavior on opacity {
            NumberAnimation { duration: 150 }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 8

        // Header: 사용자명과 역할 뱃지
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Text {
                text: root.username
                color: Theme.fontColor
                font.pixelSize: 18
                font.bold: true
                Layout.fillWidth: true
                elide: Text.ElideRight
            }

            // Role Badge
            Rectangle {
                visible: root.role === "admin"
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
                color: root.isOnline ? "#4caf50" : "#888888"
            }
        }

        // Email
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            
            Text {
                text: "📧"
                font.pixelSize: 14
            }
            
            Text {
                text: root.email
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
            visible: root.ipAddress !== ""
            
            Text {
                text: "🌐"
                font.pixelSize: 14
            }
            
            Text {
                text: root.ipAddress
                color: Theme.isDark ? "#aaaaaa" : "#666666"
                font.pixelSize: 13
            }
        }

        // Active Cameras Count
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            
            Text {
                text: "📹"
                font.pixelSize: 14
            }
            
            Text {
                text: qsTr("Active Cameras: %1").arg(root.activeCameras)
                color: Theme.isDark ? "#aaaaaa" : "#666666"
                font.pixelSize: 13
            }
        }

        // Last Login
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            visible: root.lastLogin !== ""
            
            Text {
                text: "🕒"
                font.pixelSize: 14
            }
            
            Text {
                text: qsTr("Last Login: %1").arg(root.lastLogin)
                color: Theme.isDark ? "#888888" : "#999999"
                font.pixelSize: 11
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
        }
    }
}
