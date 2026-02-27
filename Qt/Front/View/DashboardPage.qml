import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front
import AnoMap.back // Import BackEnd to access Video type if implicitly needed

Item {
    id: root

    signal requestPage(string pageName)

    Rectangle {
        anchors.fill: parent
        color: Theme.bgPrimary

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 20

            RowLayout {
                Layout.fillWidth: true

                Text {
                    text: "Dashboard"
                    color: Theme.fontColor
                    font.pixelSize: 32
                    Layout.alignment: Qt.AlignVCenter
                }

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    text: "Settings"
                    onClicked: root.requestPage("Settings")
                }

                Button {
                    text: "Logout"
                    onClicked: root.requestPage("Login")
                }
            }

            GridView {
                id: cameraGrid
                Layout.fillWidth: true
                Layout.fillHeight: true

                cellWidth: 340 // 320 width + 20 spacing
                cellHeight: 260 // 240 height + 20 spacing

                model: cameraModel // Bound via context property set in backend.cpp

                delegate: CameraCard {
                    // Implicitly passed model data from Card abstract list model
                    title: model.title
                    rtspUrl: model.rtspUrl
                    isOnline: model.isOnline // Added via IsOnlineRole
                }

                clip: true
            }
        }
    }
}
