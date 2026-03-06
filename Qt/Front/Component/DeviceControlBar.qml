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

    // ── Motor 상태 ──────────────────────────────────────────────────────────
    // motorAuto: 사용자가 수동으로 켠 Auto 모드
    // WASD 를 누르면:
    //   1) 방향 패킷 즉시 전송
    //   2) motorAuto 를 false 로 강제 (auto → unauto)
    //   3) 1초 타이머 리셋 → 1초 후 현재 auto 상태 패킷 전송
    property bool motorAuto: false

    property bool irOn: false
    property bool irAuto: false
    property bool heaterOn: false
    property bool heaterAuto: false

    signal closeRequested

    // ── 1초 후 motor auto 상태 전송 타이머 ──────────────────────────────────
    // WASD 입력이 연속으로 들어올 때마다 타이머가 restart() 되므로
    // 마지막 WASD 입력 후 정확히 1초 뒤에만 패킷이 나간다.
    Timer {
        id: motorAutoTimer
        interval: 1000
        repeat: false
        onTriggered: {
            // 1초 후 현재 motorAuto 상태를 서버에 전송
            _motor(root.motorAuto ? "auto" : "unauto");
        }
    }

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

        // ── 헤더 (타이틀 + 닫기) ─────────────────────────────────────────────
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

        // ── Motor 섹션 ────────────────────────────────────────────────────────
        Column {
            visible: root.hasMotor
            width: parent.width
            spacing: 4

            Text {
                text: "Motor"
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
                    lbl: "U"
                    onTapped: _wasd("w")
                }
                Item {
                    width: 38
                    height: 38
                }

                Btn {
                    lbl: "L"
                    onTapped: _wasd("a")
                }
                Item {
                    width: 38
                    height: 38
                }
                Btn {
                    lbl: "R"
                    onTapped: _wasd("d")
                }

                Item {
                    width: 38
                    height: 38
                }
                Btn {
                    lbl: "D"
                    onTapped: _wasd("s")
                }
                Item {
                    width: 38
                    height: 38
                }
            }

            // Auto 토글 — 사용자가 수동으로만 켤 수 있음
            // WASD 누르면 자동으로 꺼짐
            ToggleBtn {
                width: parent.width
                lbl: root.motorAuto ? "Motor Auto  ON" : "Motor Auto OFF"
                active: root.motorAuto
                onTapped: {
                    root.motorAuto = !root.motorAuto;
                    // Auto ↔ unAuto 를 수동으로 바꿀 때도 즉시 패킷 전송
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

        // ── IR 섹션 ───────────────────────────────────────────────────────────
        Column {
            visible: root.hasIr
            width: parent.width
            spacing: 4

            Text {
                text: "IR"
                color: "#8888aa"
                font.pixelSize: 10
            }
            Row {
                spacing: 6
                ToggleBtn {
                    lbl: root.irOn ? "IR ON" : "IR OFF"
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

        // ── Heater 섹션 ───────────────────────────────────────────────────────
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

    function _wasd(dir) {
        // 1. 방향 즉시 전송
        _motor(dir);

        // 2. auto 상태 강제 해제 (만약 Auto 상태였다면 즉시 unauto 전송)
        if (root.motorAuto) {
            root.motorAuto = false;
            _motor("unauto");
        }

        // 3. 1초 타이머 재시작 (마지막 누른 시점으로부터 1초 뒤 현재 상태 다시 전송)
        motorAutoTimer.restart();
    }

    function _motor(v) {
        networkBridge.sendDevice(root.deviceIp, v, "", "");
    }
    function _ir(v) {
        networkBridge.sendDevice(root.deviceIp, "", v, "");
    }
    function _heater(v) {
        networkBridge.sendDevice(root.deviceIp, "", "", v);
    }

    // ── Btn: 방향키 버튼 ────────────────────────────────────────────────────
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

    // ── ToggleBtn: 상태 토글 버튼 ────────────────────────────────────────────
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
