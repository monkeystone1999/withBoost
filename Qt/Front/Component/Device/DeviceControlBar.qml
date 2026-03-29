import QtQuick
import QtQuick.Controls
import AnoMap.Front

// Floating device controller shown from Dashboard/Device pages.
Rectangle {
    id: rootItem

    property string deviceIp: ""
    property bool hasMotor: true
    property bool hasIr: false
    property bool hasHeater: false

    property bool motorAuto: false
    property bool irOn: false
    property bool irAuto: false
    property bool heaterOn: false
    property bool heaterAuto: false

    signal closeRequested
    signal sendDeviceCmd(string ip, string motor, string ir, string heater)

    width: 220
    height: col.implicitHeight + 20
    radius: 10
    color: "#E8101020"
    border.color: "#55aaaaff"
    border.width: 1

    MouseArea {
        anchors.fill: parent
        drag.target: rootItem
        drag.minimumX: 0
        drag.minimumY: 0
    }

    Column {
        id: col
        anchors {
            left: parent.left
            right: parent.right
            top: parent.top
            margins: 10
        }
        spacing: 8

        Row {
            width: parent.width

            Text {
                text: "Device Control"
                color: "#ccccff"
                font.pixelSize: 11
                font.bold: true
                width: parent.width - 24
            }

            Rectangle {
                width: 20
                height: 20
                radius: 10
                color: closeHover.containsMouse ? "#66ff4444" : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "x"
                    color: "#aaaaaa"
                    font.pixelSize: 11
                }

                MouseArea {
                    id: closeHover
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: rootItem.closeRequested()
                }
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: "#33ffffff"
        }

        Column {
            visible: rootItem.hasMotor
            width: parent.width
            spacing: 4

            Text {
                text: "PTZ Control"
                color: "#8888aa"
                font.pixelSize: 10
            }

            Grid {
                columns: 3
                spacing: 4
                anchors.horizontalCenter: parent.horizontalCenter

                Item {
                    width: 38
                    height: 38
                }
                ControlButton {
                    lbl: "\u2191"
                    onTapped: _wasd("w")
                }
                Item {
                    width: 38
                    height: 38
                }
                ControlButton {
                    lbl: "\u2190"
                    onTapped: _wasd("a")
                }
                Item {
                    width: 38
                    height: 38
                }
                ControlButton {
                    lbl: "\u2192"
                    onTapped: _wasd("d")
                }
                Item {
                    width: 38
                    height: 38
                }
                ControlButton {
                    lbl: "\u2193"
                    onTapped: _wasd("s")
                }
                Item {
                    width: 38
                    height: 38
                }
            }

            ToggleButton {
                width: parent.width
                lbl: rootItem.motorAuto ? "Motor Auto  ON" : "Motor Auto OFF"
                active: rootItem.motorAuto
                onTapped: {
                    rootItem.motorAuto = !rootItem.motorAuto;
                    _motor(rootItem.motorAuto ? "auto" : "unauto");
                }
            }
        }

        Rectangle {
            visible: rootItem.hasMotor && (rootItem.hasIr || rootItem.hasHeater)
            width: parent.width
            height: 1
            color: "#22ffffff"
        }

        Column {
            visible: rootItem.hasIr
            width: parent.width
            spacing: 4

            Text {
                text: "Lighting"
                color: "#8888aa"
                font.pixelSize: 10
            }
            Row {
                width: parent.width
                spacing: 6
                ToggleButton {
                    width: (parent.width - parent.spacing) / 2
                    lbl: rootItem.irOn ? "Light ON" : "Light OFF"
                    active: rootItem.irOn
                    onTapped: {
                        rootItem.irOn = !rootItem.irOn;
                        _ir(rootItem.irOn ? "on" : "off");
                    }
                }
                ToggleButton {
                    width: (parent.width - parent.spacing) / 2
                    lbl: rootItem.irAuto ? "Auto ON" : "Auto OFF"
                    active: rootItem.irAuto
                    onTapped: {
                        rootItem.irAuto = !rootItem.irAuto;
                        _ir(rootItem.irAuto ? "auto" : "unauto");
                    }
                }
            }
        }

        Rectangle {
            visible: rootItem.hasIr && rootItem.hasHeater
            width: parent.width
            height: 1
            color: "#22ffffff"
        }

        Column {
            visible: rootItem.hasHeater
            width: parent.width
            spacing: 4

            Text {
                text: "Heater"
                color: "#8888aa"
                font.pixelSize: 10
            }
            Row {
                width: parent.width
                spacing: 6
                ToggleButton {
                    width: (parent.width - parent.spacing) / 2
                    lbl: rootItem.heaterOn ? "Heater ON" : "Heater OFF"
                    active: rootItem.heaterOn
                    onTapped: {
                        rootItem.heaterOn = !rootItem.heaterOn;
                        _heater(rootItem.heaterOn ? "on" : "off");
                    }
                }
                ToggleButton {
                    width: (parent.width - parent.spacing) / 2
                    lbl: rootItem.heaterAuto ? "Auto ON" : "Auto OFF"
                    active: rootItem.heaterAuto
                    onTapped: {
                        rootItem.heaterAuto = !rootItem.heaterAuto;
                        _heater(rootItem.heaterAuto ? "auto" : "unauto");
                    }
                }
            }
        }
    }

    function _motor(v) {
        rootItem.sendDeviceCmd(rootItem.deviceIp, v, "", "");
    }
    function _wasd(dir) {
        rootItem.motorAuto = false;
        _motor(dir);
    }
    function _ir(v) {
        rootItem.sendDeviceCmd(rootItem.deviceIp, "", v, "");
    }
    function _heater(v) {
        rootItem.sendDeviceCmd(rootItem.deviceIp, "", "", v);
    }

    component ControlButton: Rectangle {
        signal tapped
        property string lbl: ""

        width: 38
        height: 38
        radius: 7
        color: buttonArea.pressed ? "#886688ff" : (buttonArea.containsMouse ? "#444488ff" : "#223344")
        border.color: "#334466"
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: parent.lbl
            color: "white"
            font.pixelSize: 14
            font.bold: true
        }

        MouseArea {
            id: buttonArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: parent.tapped()
        }
    }

    component ToggleButton: Rectangle {
        signal tapped
        property string lbl: ""
        property bool active: false

        height: 26
        radius: 6
        color: active ? "#886688ff" : (toggleArea.containsMouse ? "#333355" : "#1a1a2e")
        border.color: active ? "#6688ff" : "#334466"
        border.width: 1

        Text {
            anchors.centerIn: parent
            text: parent.lbl
            color: "white"
            font.pixelSize: 10
        }

        MouseArea {
            id: toggleArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: parent.tapped()
        }
    }
}
