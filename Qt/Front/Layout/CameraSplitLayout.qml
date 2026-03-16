import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front
import "../Component/camera"

Item {
    id: root

    property var model
    property string pageType: "AI"  // "AI" or "Device"
    property string selectedUrl: ""
    property int selectedSlotId: -1

    signal cameraSelected(string url, int slotId)
    signal actionRequested(string url)
    signal cameraRightClicked(string url, real globalX, real globalY)

    // Camera Grid (fills the entire layout, as details are handled by parent pages)
    Rectangle {
        anchors.fill: parent
        color: Theme.bgPrimary

        Text {
            visible: typeof root.model !== "undefined" && cameraGrid.count === 0
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

            model: root.model
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
                    rtspUrl: model.rtspUrl
                    isOnline: model.isOnline
                    slotId: model.slotId

                    showActionIcon: true
                    actionIconText: root.pageType === "AI" ? "AI" : "⚙"
                    draggable: false
                    highlightOnHover: root.selectedUrl === model.rtspUrl

                    // 선택된 카드 강조
                    border.width: root.selectedUrl === model.rtspUrl ? 3 : 2
                    border.color: {
                        if (root.selectedUrl === model.rtspUrl)
                            return Theme.hanwhaFirst;
                        return model.isOnline ? "#4caf50" : "#f44336";
                    }

                    onTapped: {
                        root.selectedUrl = model.rtspUrl;
                        root.selectedSlotId = model.slotId;
                        root.cameraSelected(model.rtspUrl, model.slotId);
                    }

                    onRightTapped: (sx, sy) => {
                        var pos = card.mapToItem(null, sx, sy);
                        root.cameraRightClicked(model.rtspUrl, pos.x, pos.y);
                    }

                    onActionIconTapped: {
                        root.actionRequested(model.rtspUrl);
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
