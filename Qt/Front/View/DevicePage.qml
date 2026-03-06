import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front

Item {
    id: root
    signal requestPage(string pageName)

    Rectangle {
        anchors.fill: parent
        color: Theme.bgPrimary

        Text {
            visible: deviceModel.count === 0
            anchors.centerIn: parent
            text: "No controllable devices.\nWaiting for DEVICE list from server."
            color: "#666688"
            font.pixelSize: 16
            horizontalAlignment: Text.AlignHCenter
        }

        GridView {
            id: grid
            anchors.fill: parent
            anchors.margins: 20

            // 반응형 컬럼 계산 (최소 크기 280)
            property int cols: Math.max(1, Math.floor(width / 280))
            cellWidth: width / cols
            cellHeight: 420

            model: deviceModel
            clip: true
            visible: deviceModel.count > 0

            delegate: Item {
                width: grid.cellWidth
                height: grid.cellHeight
                visible: model.isOnline

                Rectangle {
                    anchors.centerIn: parent
                    width: parent.width - 20
                    height: parent.height - 20
                    radius: 12
                    color: "#1e1e2e"
                    border.color: model.isOnline ? "#4caf50" : "#f44336"
                    border.width: 2

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 8

                        // ── Header (Title & Status) ──
                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                text: model.title
                                color: "white"
                                font.pixelSize: 18
                                font.bold: true
                                Layout.fillWidth: true
                                elide: Text.ElideRight
                            }
                            Rectangle {
                                width: 70
                                height: 22
                                radius: 11
                                color: model.isOnline ? "#2e5c2e" : "#5c2e2e"
                                Text {
                                    anchors.centerIn: parent
                                    text: model.isOnline ? "ONLINE" : "OFFLINE"
                                    color: model.isOnline ? "#4caf50" : "#f44336"
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                            }
                        }

                        Text {
                            text: "IP: " + model.deviceIp
                            color: "#aab"
                            font.pixelSize: 13
                        }

                        // ── Server Status Data ──
                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 65
                            color: "#151520"
                            radius: 8
                            border.color: "#334"
                            ColumnLayout {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 2
                                Text {
                                    text: "CPU: " + serverStatus.deviceCpu(model.deviceIp).toFixed(1) + "%"
                                    color: "#89a"
                                    font.pixelSize: 12
                                }
                                Text {
                                    text: "Mem: " + serverStatus.deviceMemory(model.deviceIp).toFixed(1) + "%"
                                    color: "#89a"
                                    font.pixelSize: 12
                                }
                                Text {
                                    text: "Temp: " + serverStatus.deviceTemp(model.deviceIp).toFixed(1) + "°C"
                                    color: "#89a"
                                    font.pixelSize: 12
                                }
                            }
                        }

                        // ── Badges ──
                        Row {
                            spacing: 8
                            Layout.topMargin: 4
                            Badge {
                                lbl: "Motor"
                                vis: model.hasMotor
                            }
                            Badge {
                                lbl: "IR"
                                vis: model.hasIr
                            }
                            Badge {
                                lbl: "Heater"
                                vis: model.hasHeater
                            }
                        }

                        Item {
                            Layout.fillHeight: true
                        } // spacer

                        // ── Control Bar ──
                        Item {
                            Layout.fillWidth: true
                            implicitHeight: ctrlLoader.item ? ctrlLoader.item.height : 260
                            Loader {
                                id: ctrlLoader
                                anchors.centerIn: parent
                                active: true
                                sourceComponent: DeviceControlBar {
                                    deviceIp: model.deviceIp
                                    hasMotor: model.hasMotor
                                    hasIr: model.hasIr
                                    hasHeater: model.hasHeater
                                    // Override visual border/bg to match card seamlessly
                                    color: "transparent"
                                    border.color: "transparent"
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    component Badge: Rectangle {
        property string lbl: ""
        property bool vis: false

        visible: vis
        width: badgeText.width + 12
        height: 20
        radius: 4
        color: "#2a2a4a"
        border.color: "#4a4a7a"

        Text {
            id: badgeText
            anchors.centerIn: parent
            text: parent.lbl
            color: "#aaccff"
            font.pixelSize: 10
            font.bold: true
        }
    }
}
