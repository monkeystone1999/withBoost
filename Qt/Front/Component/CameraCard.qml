import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front
import AnoMap.back

Rectangle {
    id: root

    // Properties driven by the Card model
    property string title: ""
    property string rtspUrl: ""
    property bool isOnline: false

    width: 320
    height: 240
    color: "#1e1e1e"
    radius: 8
    border.color: isOnline ? "#4caf50" : "#f44336" // Green if online, red if offline
    border.width: 2
    clip: true

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Title Bar
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: "#2d2d2d"
            radius: root.radius

            // To square off the bottom corners of the title bar
            Rectangle {
                width: parent.width
                height: root.radius
                color: parent.color
                anchors.bottom: parent.bottom
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10

                Text {
                    text: root.title
                    color: "white"
                    font.pixelSize: 14
                    font.bold: true
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                }

                Rectangle {
                    width: 10
                    height: 10
                    radius: 5
                    color: root.isOnline ? "#4caf50" : "#f44336"
                }
            }
        }

        // Video Content
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Video {
                id: videoItem
                anchors.fill: parent
                rtspUrl: root.rtspUrl

                Component.onCompleted: {
                    if (root.isOnline && root.rtspUrl !== "") {
                        videoItem.startStream();
                    }
                }

                onRtspUrlChanged: {
                    if (root.isOnline && root.rtspUrl !== "") {
                        videoItem.startStream();
                    }
                }
            }

            // Offline placeholder
            Rectangle {
                anchors.fill: parent
                color: "transparent"
                visible: !root.isOnline

                Text {
                    anchors.centerIn: parent
                    text: "OFFLINE"
                    color: "#f44336"
                    font.pixelSize: 24
                    font.bold: true
                }
            }
        }
    }
}
