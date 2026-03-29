import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.Front
import "../Component/Camera"

Item {
    id: rootItem

    property var model
    property string pageType: "AI"  // "AI" or "Device"
    property string selectedCameraId: ""
    property int selectedSlotId: -1

    signal cameraSelected(string cameraId, int slotId)
    signal actionRequested(string cameraId)
    signal cameraRightClicked(string cameraId, real globalX, real globalY)

    // Camera Grid (fills the entire layout, as details are handled by parent pages)
    Rectangle {
        anchors.fill: parent
        color: Theme.bgPrimary

        Text {
            visible: typeof rootItem.model !== "undefined" && cameraGrid.count === 0
            anchors.centerIn: parent
            text: qsTr("No cameras available.\nCheck Dashboard first.")
            color: "#666688"
            font.pixelSize: 16
            horizontalAlignment: Text.AlignHCenter
        }

        GridView {
            id: cameraGrid
            anchors.fill: parent
            anchors.margins: 20

            property int cols: 4  // Made it 4-cols to match Dashboard
            property real maxW_byCol: parent.width / cols
            cellWidth: Math.floor(Math.max(100, maxW_byCol))
            cellHeight: Math.floor((cellWidth - 20) * 0.75 + 20)

            model: rootItem.model
            clip: true

            delegate: Item {
                width: cameraGrid.cellWidth
                height: cameraGrid.cellHeight

                CameraCard {
                    id: card
                    anchors.centerIn: parent
                    width: Math.max(10, parent.width - 20)
                    height: width * 0.75

                    title: model.title
                    cameraId: model.cameraId
                    isOnline: model.isOnline
                    slotId: model.slotId

                    showActionIcon: true
                    actionIconText: rootItem.pageType === "AI" ? "AI" : "Device"
                    draggable: false
                    highlightOnHover: rootItem.selectedCameraId === model.cameraId

                    border.width: rootItem.selectedCameraId === model.cameraId ? 3 : 2
                    border.color: {
                        if (rootItem.selectedCameraId === model.cameraId)
                            return Theme.hanwhaFirst;
                        return model.isOnline ? "#4caf50" : "#f44336";
                    }

                    onTapped: {
                        rootItem.selectedCameraId = model.cameraId;
                        rootItem.selectedSlotId = model.slotId;
                        rootItem.cameraSelected(model.cameraId, model.slotId);
                    }

                    onRightTapped: (sx, sy) => {
                        var pos = card.mapToItem(null, sx, sy);
                        rootItem.cameraRightClicked(model.cameraId, pos.x, pos.y);
                    }

                    onActionIconTapped: {
                        rootItem.actionRequested(model.cameraId);
                    }
                }
            }

            ScrollBar.vertical: ScrollBar {
                active: true
                policy: ScrollBar.AsNeeded
            }
        }
    }

    // Helper component for detail rows
    component DetailRow: Row {
        property string label: ""
        property string value: ""
        property alias valueFont: valueText.font

        width: parent.width
        spacing: 8

        Text {
            text: parent.label
            font.pixelSize: 13
            color: Theme.isDark ? "#aaaaaa" : "#666666"
            width: 80
        }

        Text {
            id: valueText
            text: parent.value
            font.pixelSize: 13
            color: Theme.fontColor
            width: parent.width - 88
            wrapMode: Text.WrapAnywhere
        }
    }
}
