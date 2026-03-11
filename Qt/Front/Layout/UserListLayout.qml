import QtQuick
import QtQuick.Controls
import AnoMap.front
import "../Component/user"

Item {
    id: root

    property var model
    property string selectedUserId: ""

    signal userSelected(string userId)
    signal userRightClicked(string userId, real globalX, real globalY)

    ListView {
        id: userListView
        anchors.fill: parent
        anchors.margins: 20
        spacing: 12
        clip: true

        model: root.model

        delegate: UserCard {
            width: userListView.width
            
            userId: model.userId
            username: model.username
            email: model.email
            role: model.role
            isOnline: model.isOnline
            lastLogin: model.lastLogin ? model.lastLogin.toLocaleString(Qt.locale(), "yyyy-MM-dd HH:mm") : ""
            ipAddress: model.ipAddress
            activeCameras: model.activeCameras

            // 선택된 항목 강조
            border.width: root.selectedUserId === model.userId ? 3 : 2
            border.color: {
                if (root.selectedUserId === model.userId)
                    return Theme.hanwhaFirst;
                return model.isOnline ? "#4caf50" : "#888888";
            }

            onTapped: {
                root.selectedUserId = model.userId;
                root.userSelected(model.userId);
            }

            onRightTapped: (x, y) => {
                const pos = mapToItem(root, x, y);
                root.userRightClicked(model.userId, pos.x, pos.y);
            }
        }

        ScrollBar.vertical: ScrollBar {
            active: true
            policy: ScrollBar.AsNeeded
        }
    }

    // No users message
    Text {
        visible: typeof root.model !== "undefined" && root.model.count === 0
        anchors.centerIn: parent
        text: qsTr("No users available.\nWaiting for user data from server.")
        color: "#666688"
        font.pixelSize: 16
        horizontalAlignment: Text.AlignHCenter
    }
}
