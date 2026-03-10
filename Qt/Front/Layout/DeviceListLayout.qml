import QtQuick
import QtQuick.Controls
import AnoMap.front
import "../Component/device"
import "../Component/camera"

Item {
    id: root
    property var model

    signal deviceControlRequested(string ip, bool motor, bool ir, bool heater)

    ListView {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 16
        model: root.model
        clip: true

        delegate: Rectangle {
            width: ListView.view.width
            height: row.implicitHeight + 24
            radius: 10
            color: "#1a1a2e"
            border.color: model.isOnline ? "#3a7f4a" : "#7f3a3a"
            border.width: 1

            Row {
                id: row
                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                    margins: 12
                }
                spacing: 16

                CameraCard {
                    width: 240
                    height: 180
                    title: model.title
                    rtspUrl: model.rtspUrl
                    isOnline: model.isOnline
                    slotId: -1
                    cropRect: Qt.rect(0, 0, 1, 1)
                    showActionIcon: false
                    draggable: false
                    highlightOnHover: false
                }

                Column {
                    width: 220
                    spacing: 6

                    Text {
                        text: model.title
                        color: "white"
                        font.pixelSize: 15
                        font.bold: true
                    }

                    Text {
                        text: "IP: " + model.deviceIp
                        color: "#8888aa"
                        font.pixelSize: 11
                    }

                    Row {
                        spacing: 6
                        DeviceBadge {
                            labelText: "Motor"
                            isActive: model.hasMotor
                        }
                        DeviceBadge {
                            labelText: "IR"
                            isActive: model.hasIr
                        }
                        DeviceBadge {
                            labelText: "Heater"
                            isActive: model.hasHeater
                        }
                    }

                    Rectangle {
                        width: 80
                        height: 22
                        radius: 6
                        color: model.isOnline ? "#224422" : "#442222"

                        Text {
                            anchors.centerIn: parent
                            text: model.isOnline ? "ONLINE" : "OFFLINE"
                            color: model.isOnline ? "#55ff77" : "#ff5555"
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }
                }

                DeviceControlBar {
                    deviceIp: model.deviceIp
                    hasMotor: model.hasMotor
                    hasIr: model.hasIr
                    hasHeater: model.hasHeater
                    visible: true
                    onSendDeviceCmd: (ip, motor, ir, heater) => {
                        root.deviceControlRequested(ip, motor, ir, heater);
                    }
                }
            }
        }
    }
}
