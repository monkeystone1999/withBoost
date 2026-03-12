import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front
import "../Layout"
import "../Component/camera"
import "../Component/device"

Item {
    id: root
    signal requestPage(string pageName)
    signal requestClose

    property string selectedUrl: ""
    property int selectedSlotId: -1
    property string selectedIp: "" // Added for device page

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ── Left Panel: Camera Grid (70%) ──────────────────────────────
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.7
            color: Theme.bgPrimary

            CameraSplitLayout {
                anchors.fill: parent
                model: typeof cameraModel !== "undefined" ? cameraModel : null
                pageType: "Device"
                selectedUrl: root.selectedUrl
                selectedSlotId: root.selectedSlotId

                onCameraSelected: (url, slotId) => {
                    root.selectedUrl = url;
                    root.selectedSlotId = slotId;
                    // Extract IP from URL for device page
                    let ipMatch = url.match(/rtsp:\/\/(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3})/);
                    if (ipMatch && ipMatch[1]) {
                        root.selectedIp = ipMatch[1];
                    } else {
                        root.selectedIp = "";
                    }
                    console.log("Device Page: Camera selected", url, "slot", slotId, "IP", root.selectedIp);
                }

                onActionRequested: url => {
                    console.log("Device Page: Device control requested for", url);
                }

                onCameraRightClicked: (url, gx, gy) => {
                    deviceCtxMenu.targetUrl = url;
                    let mx = gx, my = gy;
                    if (mx + deviceCtxMenu.width > root.width)
                        mx = root.width - deviceCtxMenu.width - 4;
                    if (my + deviceCtxMenu.height > root.height)
                        my = root.height - deviceCtxMenu.height - 4;
                    deviceCtxMenu.x = mx;
                    deviceCtxMenu.y = my;
                    deviceCtxMenu.visible = true;
                }
            }
        }

        // ── Right Panel: Detail View (30%) ─────────────────────────────
        ColumnLayout {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.3
            spacing: 16

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.bgSecondary
                radius: 8

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 24

                    // ── Header: Node Info ────────────────────────────────
                    ColumnLayout {
                        spacing: 4
                        Text {
                            text: deviceModel.hasDevice(root.selectedIp) ? root.selectedIp : "No Device Selected"
                            color: Theme.fontColor
                            font.pixelSize: 20
                            font.bold: true
                        }
                        Text {
                            text: "Live Device Status"
                            color: Theme.hanwhaFirst
                            font.pixelSize: 12
                            font.bold: true
                        }
                    }

                    // ── Current Metrics (CPU, MEM, TEMP) ──────────────────
                    GridLayout {
                        columns: 3
                        Layout.fillWidth: true
                        columnSpacing: 10

                        StatusCard {
                            title: "CPU"
                            value: deviceModel.hasDevice(root.selectedIp) ? deviceModel.cpu(root.selectedIp).toFixed(1) + "%" : "--"
                        }
                        StatusCard {
                            title: "Memory"
                            value: deviceModel.hasDevice(root.selectedIp) ? deviceModel.memory(root.selectedIp).toFixed(1) + "%" : "--"
                        }
                        StatusCard {
                            title: "Temp"
                            value: deviceModel.hasDevice(root.selectedIp) ? deviceModel.temp(root.selectedIp).toFixed(1) + "°C" : "--"
                        }
                    }

                    // ── PTZ & Device Controls ────────────────────────────
                    DeviceControlPanel {
                        Layout.fillWidth: true
                        deviceIp: root.selectedIp
                        hasMotor: deviceModel.hasDevice(root.selectedIp) ? deviceModel.data(deviceModel.index(selectedRow, 0), DeviceModel.HasMotorRole) : false
                        hasIr: deviceModel.hasDevice(root.selectedIp) ? deviceModel.data(deviceModel.index(selectedRow, 0), DeviceModel.HasIrRole) : false
                        hasHeater: deviceModel.hasDevice(root.selectedIp) ? deviceModel.data(deviceModel.index(selectedRow, 0), DeviceModel.HasHeaterRole) : false

                        property int selectedRow: -1
                        Component.onCompleted: {
                            updateSelectedRow();
                        }

                        function updateSelectedRow() {
                            for (var i = 0; i < deviceModel.rowCount(); i++) {
                                if (deviceModel.data(deviceModel.index(i, 0), DeviceModel.IpRole) === root.selectedIp) {
                                    selectedRow = i;
                                    break;
                                }
                            }
                        }

                        Connections {
                            target: root
                            function onSelectedIpChanged() {
                                updateSelectedRow();
                            }
                        }
                    }

                    // ── History List (Last 20) ───────────────────────────
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 12

                        Text {
                            text: "History (Last 20 Snapshots)"
                            color: Theme.fontColor
                            font.pixelSize: 14
                            font.bold: true
                        }

                        ListView {
                            id: historyList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            spacing: 4
                            model: root.historyData

                            delegate: Rectangle {
                                width: historyList.width
                                height: 32
                                color: Theme.isDark ? "#2d2d2d" : "#f5f5f7"
                                radius: 4

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    Text {
                                        text: Qt.formatDateTime(new Date(modelData.timestamp), "hh:mm:ss")
                                        color: Theme.fontColor
                                        font.pixelSize: 11
                                        Layout.preferredWidth: 60
                                    }
                                    Text {
                                        text: "C: " + modelData.cpu.toFixed(1) + "%"
                                        color: Theme.fontColor
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                    }
                                    Text {
                                        text: "M: " + modelData.memory.toFixed(1) + "%"
                                        color: Theme.fontColor
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                    }
                                    Text {
                                        text: modelData.temp.toFixed(1) + "°C"
                                        color: Theme.fontColor
                                        font.pixelSize: 11
                                        Layout.preferredWidth: 40
                                    }
                                }
                            }
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    visible: root.selectedIp === ""
                    text: "Click a camera card\nto view its device capabilities."
                    color: Theme.isDark ? "#555" : "#aaa"
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }

    // ── Logic: Periodic History Fetch ────────────────────────────────────────
    property var historyData: []

    Timer {
        interval: 5000
        running: !!root.selectedIp && root.visible
        repeat: true
        triggeredOnStart: true
        onTriggered: {
            root.historyData = deviceModel.getHistory(root.selectedIp);
        }
    }

    onSelectedIpChanged: {
        root.historyData = deviceModel.getHistory(root.selectedIp);
    }

    // ── Shared Component (StatusCard) ────────────────────────────────────────
    component StatusCard: Rectangle {
        property string title: ""
        property string value: ""
        Layout.fillWidth: true
        height: 60
        color: Theme.isDark ? "#2d2d2d" : "#f5f5f7"
        radius: 6
        ColumnLayout {
            anchors.centerIn: parent
            spacing: 2
            Text {
                text: title
                color: "#888"
                font.pixelSize: 10
                Layout.alignment: Qt.AlignHCenter
            }
            Text {
                text: value
                color: Theme.fontColor
                font.pixelSize: 14
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }
        }
    }

    // Context Menu
    MouseArea {
        anchors.fill: parent
        z: 399
        visible: deviceCtxMenu.visible
        onClicked: deviceCtxMenu.visible = false
    }

    DeviceContextMenu {
        id: deviceCtxMenu
        visible: false
        z: 400
        onDeviceControlRequested: url => {
            console.log("Device control from context menu for", url);
        }
        onAiControlRequested: url => {
            console.log("AI control from context menu for", url);
        }
    }
}
