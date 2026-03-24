import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.Front
import "../layouts"
import "../components/camera"
import "../components/device"

Item {
    id: rootItem
    signal requestPage(string pageName)
    signal requestClose

    property int selectedSlotId: -1
    property string selectedIp: ""
    property string selectedCameraId: ""  // cameraId for sensor LiveGraphs

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

                onActionRequested: cameraId => {}

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

        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.3
            color: Theme.bgSecondary

            Text {
                anchors.centerIn: parent
                visible: rootItem.selectedIp === ""
                text: "Click a camera card\nto view device details."
                color: Theme.isDark ? "#555" : "#aaa"
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 13
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12
                visible: rootItem.selectedIp !== ""

                ColumnLayout {
                    spacing: 4
                    Text {
                        text: rootItem.selectedIp !== "" ? rootItem.selectedIp : "No Device"
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

                GridLayout {
                    columns: 3
                    Layout.fillWidth: true
                    columnSpacing: 8

                    StatusCard {
                        title: "CPU"
                        value: rootItem.selectedIp !== "" && typeof deviceModel !== "undefined" && deviceModel.hasDevice(rootItem.selectedIp) ? deviceModel.cpu(rootItem.selectedIp).toFixed(1) + "%" : "--"
                    }
                    StatusCard {
                        title: "Memory"
                        value: rootItem.selectedIp !== "" && typeof deviceModel !== "undefined" && deviceModel.hasDevice(rootItem.selectedIp) ? deviceModel.memory(rootItem.selectedIp).toFixed(1) + "%" : "--"
                    }
                    StatusCard {
                        title: "Temp"
                        value: rootItem.selectedIp !== "" && typeof deviceModel !== "undefined" && deviceModel.hasDevice(rootItem.selectedIp) ? deviceModel.temp(rootItem.selectedIp).toFixed(1) + "¡ÆC" : "--"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    visible: rootItem.selectedCameraId !== ""

                    Text {
                        text: "Camera Sensors"
                        color: "#888"
                        font.pixelSize: 10
                    }

                    LiveGraph {
                        Layout.fillWidth: true
                        height: 55
                        title: "Humidity"
                        unit: "%"
                        lineColor: "#44aaff"
                        isCamera: true
                        targetCameraId: rootItem.selectedCameraId
                        field: "hum"
                    }
                    LiveGraph {
                        Layout.fillWidth: true
                        height: 55
                        title: "Light"
                        unit: "lx"
                        lineColor: "#ffff44"
                        isCamera: true
                        targetCameraId: rootItem.selectedCameraId
                        field: "light"
                    }
                    LiveGraph {
                        Layout.fillWidth: true
                        height: 55
                        title: "Tilt"
                        unit: "¡Æ"
                        lineColor: "#ff44aa"
                        isCamera: true
                        targetCameraId: rootItem.selectedCameraId
                        field: "tilt"
                    }
                    LiveGraph {
                        Layout.fillWidth: true
                        height: 55
                        title: "Temp"
                        unit: "¡ÆC"
                        lineColor: "#ffaa44"
                        isCamera: true
                        targetCameraId: rootItem.selectedCameraId
                        field: "tmp"
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 16
                    visible: rootItem.selectedIp !== ""

                    Text {
                        text: qsTr("Device Controls")
                        color: Theme.fontColor
                        font.pixelSize: 16
                        font.bold: true
                    }

                    GridLayout {
                        Layout.alignment: Qt.AlignHCenter
                        columns: 3
                        rowSpacing: 10
                        columnSpacing: 10

                        // Up row
                        Item {
                            width: 60
                            height: 60
                        }
                        Rectangle {
                            width: 60
                            height: 60
                            radius: 8
                            color: upArea.pressed ? "#aaaaaa" : "#333333"
                            border.color: "#555"
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: "\u2191"
                                color: "white"
                                font.pixelSize: 24
                                font.bold: true
                            }
                            MouseArea {
                                id: upArea
                                anchors.fill: parent
                                onPressed: {
                                    if (autoSwitch.checked) {
                                        autoSwitch.checked = false; // Triggers onToggled which handles the timer
                                    }
                                    if (typeof networkBridge !== "undefined")
                                        networkBridge.sendDevice(rootItem.selectedIp, "w", "", "");
                                }
                            }
                        }
                        Item {
                            width: 60
                            height: 60
                        }

                        // Middle row
                        Rectangle {
                            width: 60
                            height: 60
                            radius: 8
                            color: leftArea.pressed ? "#aaaaaa" : "#333333"
                            border.color: "#555"
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: "\u2190"
                                color: "white"
                                font.pixelSize: 24
                                font.bold: true
                            }
                            MouseArea {
                                id: leftArea
                                anchors.fill: parent
                                onPressed: {
                                    if (autoSwitch.checked) {
                                        autoSwitch.checked = false;
                                    }
                                    if (typeof networkBridge !== "undefined")
                                        networkBridge.sendDevice(rootItem.selectedIp, "a", "", "");
                                }
                            }
                        }
                        Rectangle {
                            width: 60
                            height: 60
                            radius: 8
                            color: downArea.pressed ? "#aaaaaa" : "#333333"
                            border.color: "#555"
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: "\u2193"
                                color: "white"
                                font.pixelSize: 24
                                font.bold: true
                            }
                            MouseArea {
                                id: downArea
                                anchors.fill: parent
                                onPressed: {
                                    if (autoSwitch.checked) {
                                        autoSwitch.checked = false;
                                    }
                                    if (typeof networkBridge !== "undefined")
                                        networkBridge.sendDevice(rootItem.selectedIp, "s", "", "");
                                }
                            }
                        }
                        Rectangle {
                            width: 60
                            height: 60
                            radius: 8
                            color: rightArea.pressed ? "#aaaaaa" : "#333333"
                            border.color: "#555"
                            border.width: 1
                            Text {
                                anchors.centerIn: parent
                                text: "\u2192"
                                color: "white"
                                font.pixelSize: 24
                                font.bold: true
                            }
                            MouseArea {
                                id: rightArea
                                anchors.fill: parent
                                onPressed: {
                                    if (autoSwitch.checked) {
                                        autoSwitch.checked = false;
                                    }
                                    if (typeof networkBridge !== "undefined")
                                        networkBridge.sendDevice(rootItem.selectedIp, "d", "", "");
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        height: 1
                        color: Theme.isDark ? "#3A3A3C" : "#D1D1D6"
                        Layout.topMargin: 8
                        Layout.bottomMargin: 8
                    }

                    // Sensor / Heater Switches (Row)
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 20

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Text {
                                text: qsTr("Lighting")
                                color: Theme.isDark ? "#aaaaaa" : "#666666"
                                font.pixelSize: 14
                            }
                            Switch {
                                id: lightSwitch
                                onToggled: {
                                    if (typeof networkBridge !== "undefined")
                                        networkBridge.sendDevice(rootItem.selectedIp, "", checked ? "on" : "off", "");
                                }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Text {
                                text: qsTr("Heater")
                                color: Theme.isDark ? "#aaaaaa" : "#666666"
                                font.pixelSize: 14
                            }
                            Switch {
                                id: helperSwitch
                                onToggled: {
                                    if (typeof networkBridge !== "undefined")
                                        networkBridge.sendDevice(rootItem.selectedIp, "", "", checked ? "on" : "off");
                                }
                            }
                        }
                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 8
                            Text {
                                text: qsTr("CameraAuto")
                                color: Theme.isDark ? "#aaaaaa" : "#666666"
                                font.pixelSize: 14
                            }
                            Switch {
                                id: autoSwitch
                                onToggled: {
                                    if (typeof networkBridge !== "undefined" && rootItem.selectedIp !== "")
                                        networkBridge.sendDevice(rootItem.selectedIp, checked ? "auto" : "unauto", "", "");
                                }
                            }
                        }
                    }
                }

                Item {
                    Layout.fillHeight: true
                }
            }
        }
    }

    component StatusCard: Rectangle {
        property string title: ""
        property string value: ""
        Layout.fillWidth: true
        height: 56
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
        onDeviceControlRequested: cameraId => {}
        onAiControlRequested: cameraId => {}
    }
}
