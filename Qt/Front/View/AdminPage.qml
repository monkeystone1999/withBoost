import QtQuick
import QtQuick.Controls
import AnoMap.front
import "../Layout"

// ══════════════════════════════════════════════════════════════════════════════
// AdminPage — Admin 전용 사용자 관리 페이지
// UserListLayout을 통해 접속 중인 사용자 리스트를 표시
// ══════════════════════════════════════════════════════════════════════════════

Item {
    id: root
    signal requestPage(string pageName)
    signal requestClose

    Rectangle {
        anchors.fill: parent
        color: Theme.bgPrimary

        // ── Header ──────────────────────────────────────────────────────
        Rectangle {
            id: header
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 56
            color: "transparent"

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 24
                anchors.verticalCenter: parent.verticalCenter
                text: "👤  User Management"
                font.pixelSize: 20
                font.bold: true
                color: Theme.fontColor
            }

            // 사용자 수 표시
            Text {
                anchors.right: parent.right
                anchors.rightMargin: 24
                anchors.verticalCenter: parent.verticalCenter
                text: typeof userModel !== "undefined" ? qsTr("Total: %1 users").arg(userModel.count) : ""
                font.pixelSize: 14
                color: Theme.isDark ? "#888" : "#666"
            }

            // Divider
            Rectangle {
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                height: 1
                color: Theme.isDark ? "#3A3A3C" : "#D1D1D6"
            }
        }

        // ── User List ───────────────────────────────────────────────────
        UserListLayout {
            anchors.top: header.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            model: typeof userModel !== "undefined" ? userModel : null

            onUserSelected: userId => {
                console.log("[AdminPage] User selected:", userId);
            }
        }
    }
}
