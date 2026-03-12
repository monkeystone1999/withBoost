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

    // Main horizontal split: 70% for camera grid, 30% for details
    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ── Left Panel: Camera Grid (70%) ─────────────────────────────────
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.7
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
                        cropRect: model.cropRect

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

        // ── Divider ──────────────────────────────────────────────────────
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 2
            color: Theme.isDark ? "#3A3A3C" : "#D1D1D6"
        }

        // ── Right Panel: Detail View (30%) ──────────────────────────────
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.3
            color: Theme.bgSecondary

            Flickable {
                anchors.fill: parent
                contentHeight: detailColumn.implicitHeight
                clip: true

                Column {
                    id: detailColumn
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 20
                    spacing: 16

                    // Header
                    Text {
                        width: parent.width
                        text: root.selectedUrl === "" ? qsTr("Camera Details") : qsTr("Selected Camera")
                        font.pixelSize: 20
                        font.bold: true
                        color: Theme.fontColor
                    }

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: Theme.isDark ? "#3A3A3C" : "#D1D1D6"
                    }

                    // No selection message
                    Text {
                        visible: root.selectedUrl === ""
                        width: parent.width
                        text: qsTr("Click on a camera card to view details")
                        color: "#666688"
                        font.pixelSize: 14
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignHCenter
                        topPadding: 40
                    }

                    // Selected camera details
                    Column {
                        visible: root.selectedUrl !== ""
                        width: parent.width
                        spacing: 12

                        // Title
                        DetailRow {
                            label: qsTr("Title:")
                            value: {
                                if (typeof root.model === "undefined" || root.selectedSlotId < 0)
                                    return "-";
                                return root.model.titleForSlot(root.selectedSlotId);
                            }
                        }

                        // RTSP URL
                        DetailRow {
                            label: qsTr("RTSP URL:")
                            value: root.selectedUrl
                            valueFont.pixelSize: 11
                        }

                        // Slot ID
                        DetailRow {
                            label: qsTr("Slot ID:")
                            value: root.selectedSlotId.toString()
                        }

                        // Online Status
                        Row {
                            width: parent.width
                            spacing: 8

                            Text {
                                text: qsTr("Status:")
                                font.pixelSize: 13
                                color: Theme.isDark ? "#aaaaaa" : "#666666"
                                width: 80
                            }

                            Rectangle {
                                width: 12
                                height: 12
                                radius: 6
                                anchors.verticalCenter: parent.verticalCenter
                                color: {
                                    if (typeof root.model === "undefined" || root.selectedSlotId < 0)
                                        return "#888888";
                                    return root.model.isOnlineForSlot(root.selectedSlotId) ? "#4caf50" : "#f44336";
                                }
                            }

                            Text {
                                text: {
                                    if (typeof root.model === "undefined" || root.selectedSlotId < 0)
                                        return "-";
                                    return root.model.isOnlineForSlot(root.selectedSlotId) ? qsTr("Online") : qsTr("Offline");
                                }
                                font.pixelSize: 13
                                color: Theme.fontColor
                                anchors.verticalCenter: parent.verticalCenter
                            }
                        }

                        // Device Controls (Device page only)
                        Column {
                            visible: root.pageType === "Device" && typeof deviceModel !== "undefined"
                            width: parent.width
                            spacing: 8

                            property string deviceIp: typeof deviceModel !== "undefined" ? deviceModel.deviceIp(root.selectedUrl) : ""

                            Rectangle {
                                width: parent.width
                                height: 1
                                color: Theme.isDark ? "#3A3A3C" : "#D1D1D6"
                            }

                            Text {
                                text: qsTr("Device Controls")
                                font.pixelSize: 15
                                font.bold: true
                                color: Theme.fontColor
                            }

                            // Motor Control
                            Row {
                                spacing: 8
                                visible: typeof deviceModel !== "undefined" && deviceModel.hasMotor(root.selectedUrl)

                                Text {
                                    text: qsTr("Motor:")
                                    font.pixelSize: 13
                                    color: Theme.isDark ? "#aaaaaa" : "#666666"
                                    anchors.verticalCenter: parent.verticalCenter
                                }

                                BasicButton {
                                    text: "\u2190"
                                    width: 40
                                    height: 32
                                    onClicked: {
                                        if (typeof networkBridge !== "undefined")
                                            networkBridge.sendDevice(parent.parent.parent.deviceIp, "a", "", "");
                                    }
                                }
                                BasicButton {
                                    text: "\u2191"
                                    width: 40
                                    height: 32
                                    onClicked: {
                                        if (typeof networkBridge !== "undefined")
                                            networkBridge.sendDevice(parent.parent.parent.deviceIp, "w", "", "");
                                    }
                                }
                                BasicButton {
                                    text: "\u2193"
                                    width: 40
                                    height: 32
                                    onClicked: {
                                        if (typeof networkBridge !== "undefined")
                                            networkBridge.sendDevice(parent.parent.parent.deviceIp, "s", "", "");
                                    }
                                }
                                BasicButton {
                                    text: "\u2192"
                                    width: 40
                                    height: 32
                                    onClicked: {
                                        if (typeof networkBridge !== "undefined")
                                            networkBridge.sendDevice(parent.parent.parent.deviceIp, "d", "", "");
                                    }
                                }
                            }

                            // IR Control
                            Row {
                                spacing: 8
                                visible: typeof deviceModel !== "undefined" && deviceModel.hasIr(root.selectedUrl)

                                Text {
                                    text: qsTr("IR:")
                                    font.pixelSize: 13
                                    color: Theme.isDark ? "#aaaaaa" : "#666666"
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 60
                                }

                                Switch {
                                    id: irSwitch
                                    anchors.verticalCenter: parent.verticalCenter
                                    onToggled: {
                                        if (typeof networkBridge !== "undefined") {
                                            var ip = typeof deviceModel !== "undefined" ? deviceModel.deviceIp(root.selectedUrl) : "";
                                            networkBridge.sendDevice(ip, "", checked ? "on" : "off", "");
                                        }
                                    }
                                }
                            }

                            // Heater Control
                            Row {
                                spacing: 8
                                visible: typeof deviceModel !== "undefined" && deviceModel.hasHeater(root.selectedUrl)

                                Text {
                                    text: qsTr("Heater:")
                                    font.pixelSize: 13
                                    color: Theme.isDark ? "#aaaaaa" : "#666666"
                                    anchors.verticalCenter: parent.verticalCenter
                                    width: 60
                                }

                                Switch {
                                    id: heaterSwitch
                                    anchors.verticalCenter: parent.verticalCenter
                                    onToggled: {
                                        if (typeof networkBridge !== "undefined") {
                                            var ip = typeof deviceModel !== "undefined" ? deviceModel.deviceIp(root.selectedUrl) : "";
                                            networkBridge.sendDevice(ip, "", "", checked ? "on" : "off");
                                        }
                                    }
                                }
                            }
                        }

                        // AI Information (AI 페이지인 경우)
                        Column {
                            visible: root.pageType === "AI"
                            width: parent.width
                            spacing: 8

                            Rectangle {
                                width: parent.width
                                height: 1
                                color: Theme.isDark ? "#3A3A3C" : "#D1D1D6"
                            }

                            Text {
                                text: qsTr("AI Event History")
                                font.pixelSize: 15
                                font.bold: true
                                color: Theme.fontColor
                            }

                            Text {
                                text: qsTr("No recent events")
                                font.pixelSize: 12
                                color: "#666688"
                            }

                            BasicButton {
                                text: qsTr("View Full History")
                                width: parent.width
                                height: 36
                            }
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar {
                    active: true
                    policy: ScrollBar.AsNeeded
                }
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
