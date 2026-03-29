import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.Front

Rectangle {
    id: rootItem

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

        GridLayout {
            Layout.alignment: Qt.AlignHCenter
            columns: 3
            rows: 3
            rowSpacing: 8
            columnSpacing: 8

            visible: rootItem.hasMotor

            // Row 1
            Item {
                width: 40
                height: 40
            }
            PtzButton {
                text: "UP"
                onClicked: sendMotorCommand("up")
            }
            Item {
                width: 40
                height: 40
            }

            // Row 2
            PtzButton {
                text: "Left"
                onClicked: sendMotorCommand("left")
            }
            PtzButton {
                text: "Home"
                onClicked: sendMotorCommand("home")
            }
            PtzButton {
                text: "Right"
                onClicked: sendMotorCommand("right")
            }

            // Row 3
            Item {
                width: 40
                height: 40
            }
            PtzButton {
                text: "Down"
                onClicked: sendMotorCommand("down")
            }
            Item {
                width: 40
                height: 40
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 20

            ColumnLayout {
                visible: rootItem.hasIr
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
                visible: rootItem.hasHeater
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

    function sendMotorCommand(direction) {
        if (typeof networkBridge === "undefined" || !rootItem.deviceIp)
            return;
        networkBridge.sendDevice(rootItem.deviceIp, direction, "", "");
    }

    function sendIrCommand(state) {
        if (typeof networkBridge === "undefined" || !rootItem.deviceIp)
            return;
        networkBridge.sendDevice(rootItem.deviceIp, "", state, "");
    }

    function sendHeaterCommand(state) {
        if (typeof networkBridge === "undefined" || !rootItem.deviceIp)
            return;
        networkBridge.sendDevice(rootItem.deviceIp, "", "", state);
    }

    component PtzButton: Rectangle {
        property alias text: buttonLabel.text
        signal clicked

        width: 40
        height: 40
        color: buttonHoverArea.containsMouse ? Theme.hanwhaFirst : (Theme.isDark ? "#2d2d2d" : "#e5e5ea")
        radius: 4
        border.color: Theme.isDark ? "#444" : "#ccc"
        border.width: 1

        Text {
            id: buttonLabel
            anchors.centerIn: parent
            color: Theme.fontColor
            font.pixelSize: 16
        }

        MouseArea {
            id: buttonHoverArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }
}
