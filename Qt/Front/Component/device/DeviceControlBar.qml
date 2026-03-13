import QtQuick
import QtQuick.Controls
import AnoMap.front

// Floating device controller shown from Dashboard/Device pages.
Rectangle {
    id: root

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
        drag.target: root
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
                    onClicked: root.closeRequested()
                }
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: "#33ffffff"
        }

        Column {
            visible: root.hasMotor
            width: parent.width
            spacing: 4

            Text {
                text: qsTr("PTZ 방향키")
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
                Btn {
                    lbl: "\u2191"
                    onTapped: _wasd("w")
                }
                Item {
                    width: 38
                    height: 38
                }
                Btn {
                    lbl: "\u2190"
                    onTapped: _wasd("a")
                }
                Item {
                    width: 38
                    height: 38
                }
                Btn {
                    lbl: "\u2192"
                    onTapped: _wasd("d")
                }
                Item {
                    width: 38
                    height: 38
                }
                Btn {
                    lbl: "\u2193"
                    onTapped: _wasd("s")
                }
                Item {
                    width: 38
                    height: 38
                }
            }

            ToggleBtn {
                width: parent.width
                lbl: root.motorAuto ? "Motor Auto ON" : "Motor Auto OFF"
                active: root.motorAuto
                onTapped: {
                    root.motorAuto = !root.motorAuto;
                    _motor(root.motorAuto ? "auto" : "unauto");
                }
            }
        }

        Rectangle {
            visible: root.hasMotor && (root.hasIr || root.hasHeater)
            width: parent.width
            height: 1
            color: "#22ffffff"
        }

        Column {
            visible: root.hasIr
            width: parent.width
            spacing: 4

            Text {
                text: qsTr("조도센서")
                color: "#8888aa"
                font.pixelSize: 10
            }
            Row {
                spacing: 6
                ToggleBtn {
                    lbl: root.irOn ? "센서 ON" : "센서 OFF"
                    active: root.irOn
                    onTapped: {
                        root.irOn = !root.irOn;
                        _ir(root.irOn ? "on" : "off");
                    }
                }
                ToggleBtn {
                    lbl: root.irAuto ? "Auto ON" : "Auto OFF"
                    active: root.irAuto
                    onTapped: {
                        root.irAuto = !root.irAuto;
                        _ir(root.irAuto ? "auto" : "unauto");
                    }
                }
            }
        }

        Rectangle {
            visible: root.hasIr && root.hasHeater
            width: parent.width
            height: 1
            color: "#22ffffff"
        }

        Column {
            visible: root.hasHeater
            width: parent.width
            spacing: 4

            Text {
                text: "Heater"
                color: "#8888aa"
                font.pixelSize: 10
            }
            Row {
                spacing: 6
                ToggleBtn {
                    lbl: root.heaterOn ? "Heater ON" : "Heater OFF"
                    active: root.heaterOn
                    onTapped: {
                        root.heaterOn = !root.heaterOn;
                        _heater(root.heaterOn ? "on" : "off");
                    }
                }
                ToggleBtn {
                    lbl: root.heaterAuto ? "Auto ON" : "Auto OFF"
                    active: root.heaterAuto
                    onTapped: {
                        root.heaterAuto = !root.heaterAuto;
                        _heater(root.heaterAuto ? "auto" : "unauto");
                    }
                }
            }
        }
    }

    // 1초 동안 방향키 입력이 없으면 unauto 전송
    Timer {
        id: wasdTimer
        interval: 1000
        repeat: false
        onTriggered: _motor("unauto")
    }

    function _motor(v) {
        root.sendDeviceCmd(root.deviceIp, v, "", "");
    }
    function _wasd(dir) {
        root.motorAuto = false;
        _motor(dir);
        wasdTimer.restart();
    }
    function _ir(v) {
        root.sendDeviceCmd(root.deviceIp, "", v, "");
    }
    function _heater(v) {
        root.sendDeviceCmd(root.deviceIp, "", "", v);
    }

    component Btn: Rectangle {
        signal tapped
        property string lbl: ""

        width: 38
        height: 38
        radius: 7
        color: btnArea.pressed ? "#886688ff" : (btnArea.containsMouse ? "#444488ff" : "#223344")
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
            id: btnArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: parent.tapped()
        }
    }

    component ToggleBtn: Rectangle {
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
