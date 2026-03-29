import QtQuick
import QtQuick.Controls
import AnoMap.Front
import "../Component/User"

Item {
    id: rootItem

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

        model: rootItem.model

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

            border.width: rootItem.selectedUserId === model.userId ? 3 : 2
            border.color: {
                if (rootItem.selectedUserId === model.userId)
                    return Theme.hanwhaFirst;
                return model.isOnline ? "#4caf50" : "#888888";
            }

            onTapped: {
                rootItem.selectedUserId = model.userId;
                rootItem.userSelected(model.userId);
            }

            onRightTapped: (x, y) => {
                const pos = mapToItem(rootItem, x, y);
                rootItem.userRightClicked(model.userId, pos.x, pos.y);
            }
        }

        ScrollBar.vertical: ScrollBar {
            active: true
            policy: ScrollBar.AsNeeded
        }
    }

    // No users message
    Text {
        visible: typeof rootItem.model !== "undefined" && rootItem.model.count === 0
        anchors.centerIn: parent
        text: qsTr("No users available.\nWaiting for user data from server.")
        color: "#666688"
        font.pixelSize: 16
        horizontalAlignment: Text.AlignHCenter
    }
}
