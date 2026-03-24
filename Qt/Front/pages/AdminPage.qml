import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.Front
import "../layouts"
import "../components/device"

Item {
    id: rootItem
    signal requestPage(string pageName)
    signal requestClose

    Rectangle {
        anchors.fill: parent
        color: Theme.bgPrimary

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
                text: "User Management"
                font.pixelSize: 20
                font.bold: true
                color: Theme.fontColor
            }

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

        Row {
            id: serverGraphs
            anchors.top: header.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 20
            height: 140
            spacing: 20

            LiveGraph {
                width: (parent.width - 40) / 3
                height: parent.height
                title: "Server CPU"
                unit: "%"
                field: "cpu"
                isServer: true
                lineColor: "#ff4444"
            }
            LiveGraph {
                width: (parent.width - 40) / 3
                height: parent.height
                title: "Server Memory"
                unit: "%"
                field: "memory"
                isServer: true
                lineColor: "#44cc44"
            }
            LiveGraph {
                width: (parent.width - 40) / 3
                height: parent.height
                title: "Server Temp"
                unit: "¡ÆC"
                field: "temp"
                isServer: true
                lineColor: "#ffaa00"
            }
        }

        UserListLayout {
            anchors.top: serverGraphs.bottom
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
