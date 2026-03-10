import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front
import "../Component/camera"

Item {
    id: root

    // Page Name Property
    property string pageName: "AI"
    signal requestPage(string pageName)
    signal requestClose

    Rectangle {
        anchors.fill: parent
        color: Theme.bgPrimary

        Text {
            visible: typeof cameraModel !== "undefined" && cameraModel.count === 0
            anchors.centerIn: parent
            text: qsTr("No cameras available.\nCheck Dashboard first.")
            color: "#666688"
            font.pixelSize: 16
            horizontalAlignment: Text.AlignHCenter
        }

        GridView {
            id: aiGrid
            anchors.fill: parent
            anchors.margins: 20

            property int cols: 4
            property real maxW_byCol: parent.width / cols
            cellWidth: Math.floor(Math.max(100, maxW_byCol))
            cellHeight: Math.floor((cellWidth - 20) * 0.75 + 20)

            model: typeof cameraModel !== "undefined" ? cameraModel : null
            clip: true
            visible: typeof cameraModel !== "undefined" && cameraModel.count > 0

            delegate: Item {
                width: aiGrid.cellWidth
                height: aiGrid.cellHeight

                CameraCard {
                    anchors.centerIn: parent
                    width: Math.max(10, parent.width - 20)
                    height: width * 0.75

                    title: model.title
                    rtspUrl: model.rtspUrl
                    isOnline: model.isOnline
                    slotId: model.slotId
                    cropRect: model.cropRect

                    showActionIcon: true
                    actionIconText: "AI"
                    draggable: false
                    highlightOnHover: false

                    onTapped: {
                        console.log("AIPage: Card tapped, showing event history for", model.rtspUrl);
                        // TODO: Event history logic
                    }

                    onActionIconTapped: {
                        console.log("AIPage: AI icon tapped for", model.rtspUrl);
                        // TODO: AI overlay setup or configuration
                    }
                }
            }
        }
    }
}
