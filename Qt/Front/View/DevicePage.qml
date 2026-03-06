import QtQuick
import QtQuick.Controls
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

        ListView {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 16
            model: deviceModel
            clip: true
            visible: deviceModel.count > 0

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
                            Badge { lbl: "Motor"; vis: model.hasMotor }
                            Badge { lbl: "IR"; vis: model.hasIr }
                            Badge { lbl: "Heater"; vis: model.hasHeater }
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
                    }
                }

                component Badge: Rectangle {
                    property string lbl: ""
                    property bool vis: false

                    visible: vis
                    width: badgeText.width + 10
                    height: 18
                    radius: 4
                    color: "#2a2a4a"

                    Text {
                        id: badgeText
                        anchors.centerIn: parent
                        text: parent.lbl
                        color: "#8899cc"
                        font.pixelSize: 9
                    }
                }
            }
        }
    }
}
