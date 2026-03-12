import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front

Rectangle {
    id: root

    property string deviceIp: ""
    property bool hasMotor: true
    property bool hasIr: false
    property bool hasHeater: false

    height: controlColumn.implicitHeight + 40
    color: Theme.bgPrimary
    radius: 8
    border.color: Theme.isDark ? "#444" : "#ccc"
    border.width: 1

    ColumnLayout {
        id: controlColumn
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        Text {
            text: "Device Controls"
            color: Theme.fontColor
            font.pixelSize: 16
            font.bold: true
        }

        // ── 방향키 (Motor Control) ──────────────────────────
        GridLayout {
            Layout.alignment: Qt.AlignHCenter
            columns: 3
            rows: 3
            rowSpacing: 8
            columnSpacing: 8

            visible: root.hasMotor

            // Row 1
            Item {
                width: 40
                height: 40
            }
            PtzButton {
                text: "▲"
                onClicked: sendMotorCommand("up")
            }
            Item {
                width: 40
                height: 40
            }

            // Row 2
            PtzButton {
                text: "◄"
                onClicked: sendMotorCommand("left")
            }
            PtzButton {
                text: "●"
                onClicked: sendMotorCommand("home")
            }
            PtzButton {
                text: "►"
                onClicked: sendMotorCommand("right")
            }

            // Row 3
            Item {
                width: 40
                height: 40
            }
            PtzButton {
                text: "▼"
                onClicked: sendMotorCommand("down")
            }
            Item {
                width: 40
                height: 40
            }
        }

        // ── IR & Heater Control ──────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: 20

            ColumnLayout {
                visible: root.hasIr
                Text {
                    text: "IR LED"
                    color: Theme.fontColor
                    font.pixelSize: 12
                }
                Row {
                    spacing: 4
                    Button {
                        text: "ON"
                        onClicked: sendIrCommand("on")
                    }
                    Button {
                        text: "OFF"
                        onClicked: sendIrCommand("off")
                    }
                }
            }

            ColumnLayout {
                visible: root.hasHeater
                Text {
                    text: "Heater"
                    color: Theme.fontColor
                    font.pixelSize: 12
                }
                Row {
                    spacing: 4
                    Button {
                        text: "ON"
                        onClicked: sendHeaterCommand("on")
                    }
                    Button {
                        text: "OFF"
                        onClicked: sendHeaterCommand("off")
                    }
                }
            }
        }
    }

    // ── Helper Functions ─────────────────────────────────────
    function sendMotorCommand(direction) {
        if (typeof networkBridge === "undefined" || !root.deviceIp)
            return;
        networkBridge.sendDevice(root.deviceIp, direction, "", "");
    }

    function sendIrCommand(state) {
        if (typeof networkBridge === "undefined" || !root.deviceIp)
            return;
        networkBridge.sendDevice(root.deviceIp, "", state, "");
    }

    function sendHeaterCommand(state) {
        if (typeof networkBridge === "undefined" || !root.deviceIp)
            return;
        networkBridge.sendDevice(root.deviceIp, "", "", state);
    }

    // ── PTZ Button Component ─────────────────────────────────
    component PtzButton: Rectangle {
        property alias text: btnText.text
        signal clicked

        width: 40
        height: 40
        color: btnHover.containsMouse ? Theme.hanwhaFirst : (Theme.isDark ? "#2d2d2d" : "#e5e5ea")
        radius: 4
        border.color: Theme.isDark ? "#444" : "#ccc"
        border.width: 1

        Text {
            id: btnText
            anchors.centerIn: parent
            color: Theme.fontColor
            font.pixelSize: 16
        }

        MouseArea {
            id: btnHover
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }
}
