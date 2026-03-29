import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.Front
import "../Layout"
import "../Component/Camera"
import "../Component/Device"

Item {
    id: rootItem
    property string pageName: "Device"
    signal requestPage(string pageName)
    signal requestClose

    property string selectedCameraId: ""
    property int selectedSlotId: -1
    property string selectedIp: ""

    property bool pageReady: false

    StackView.onActivating: loadTimer.start()
    StackView.onDeactivating: {
        pageReady = false;
        selectedIp = "";
        selectedCameraId = "";
        selectedSlotId = -1;
    }

    Timer {
        id: loadTimer
        interval: 320
        onTriggered: rootItem.pageReady = true
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ── 70% Left Panel: CameraSplitLayout ──────────────────────────────
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.7
            color: Theme.bgPrimary

            CameraSplitLayout {
                anchors.fill: parent
                model: rootItem.pageReady && typeof cameraModel !== "undefined" ? cameraModel : null
                pageType: "Device"
                selectedCameraId: rootItem.selectedCameraId
                selectedSlotId: rootItem.selectedSlotId

                onCameraSelected: (cameraId, slotId) => {
                    rootItem.selectedCameraId = cameraId;
                    rootItem.selectedSlotId = slotId;
                    rootItem.selectedIp = (typeof deviceModel !== "undefined") ? deviceModel.deviceIp(cameraId) : "";
                }

                onCameraRightClicked: (cameraId, gx, gy) => {
                    deviceCtxMenu.targetCameraId = cameraId;
                    let mx = gx, my = gy;
                    if (mx + deviceCtxMenu.width > rootItem.width)
                        mx = rootItem.width - deviceCtxMenu.width - 4;
                    if (my + deviceCtxMenu.height > rootItem.height)
                        my = rootItem.height - deviceCtxMenu.height - 4;
                    deviceCtxMenu.x = mx;
                    deviceCtxMenu.y = my;
                    deviceCtxMenu.visible = true;
                }
            }
        }

        // ── 30% Right Panel: Device Info & Telemetry ────────────────────────
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.3
            color: Theme.bgSecondary

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 16
                visible: rootItem.selectedCameraId !== ""

                // Header
                ColumnLayout {
                    spacing: 4
                    Text {
                        text: rootItem.selectedIp !== "" ? rootItem.selectedIp : rootItem.selectedCameraId
                        color: Theme.fontColor
                        font.pixelSize: 20
                        font.bold: true
                    }
                    Text {
                        text: "Device Telemetry & Control"
                        color: Theme.hanwhaFirst
                        font.pixelSize: 12
                        font.bold: true
                    }
                }

                // Status indicators
                RowLayout {
                    spacing: 12
                    DeviceBadge {
                        label: "PTZ Auto"
                        active: typeof deviceModel !== "undefined" && deviceModel.hasMotor(rootItem.selectedCameraId)
                    }
                    DeviceBadge {
                        label: "IR On"
                        active: typeof deviceModel !== "undefined" && deviceModel.hasIr(rootItem.selectedCameraId)
                    }
                    DeviceBadge {
                        label: "Heater On"
                        active: typeof deviceModel !== "undefined" && deviceModel.hasHeater(rootItem.selectedCameraId)
                    }
                }

                // Graphs
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    contentWidth: width
                    clip: true

                    ColumnLayout {
                        width: parent.width
                        spacing: 16

                        LiveGraph {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 140
                            title: "Temperature"
                            unit: "°C"
                            colorHint: "#FF5252"
                            dataSeries: typeof deviceModel !== "undefined" ? deviceModel.getMetaHistory(rootItem.selectedCameraId, "tmp") : []
                        }

                        LiveGraph {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 140
                            title: "Humidity"
                            unit: "%"
                            colorHint: "#448AFF"
                            dataSeries: typeof deviceModel !== "undefined" ? deviceModel.getMetaHistory(rootItem.selectedCameraId, "hum") : []
                        }

                        LiveGraph {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 140
                            title: "Light"
                            unit: "lx"
                            colorHint: "#FFD740"
                            dataSeries: typeof deviceModel !== "undefined" ? deviceModel.getMetaHistory(rootItem.selectedCameraId, "light") : []
                        }

                        LiveGraph {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 140
                            title: "Tilt"
                            unit: "°"
                            colorHint: "#69F0AE"
                            dataSeries: typeof deviceModel !== "undefined" ? deviceModel.getMetaHistory(rootItem.selectedCameraId, "tilt") : []
                        }
                    }
                }

                DeviceControlPanel {
                    Layout.fillWidth: true
                    cameraId: rootItem.selectedCameraId
                }
            }

            Text {
                anchors.centerIn: parent
                visible: rootItem.selectedCameraId === ""
                text: "Click a camera card\nto view device telemetry."
                color: Theme.isDark ? "#555" : "#aaa"
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

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
        onActionRequested: (action, cameraId) => {
            console.log("Device Action", action, "requested for", cameraId);
            deviceCtxMenu.visible = false;
        }
    }
}
