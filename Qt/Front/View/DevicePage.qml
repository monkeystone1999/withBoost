import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front
import "../Layout"
import "../Component/camera"
import "../Component/device"

// ── DevicePage ──────────────────────────────────────────────────────────────
// Qt/Front: Layout + 표현만 담당.
// 비즈니스 로직(device capability 조회, 네트워크 전송)은 C++ deviceModel / networkBridge 에 위임.
// ────────────────────────────────────────────────────────────────────────────
Item {
    id: root
    signal requestPage(string pageName)
    signal requestClose

    // ── 선택 상태 ─────────────────────────────────────────────────────────────
    property string selectedUrl: ""
    property int selectedSlotId: -1
    property string selectedIp: ""

    // ── 레이지 로드: 페이지 전환 애니메이션(320ms) 이후 모델 바인딩 ─────────────
    property bool pageReady: false

    StackView.onActivating: loadTimer.start()
    StackView.onDeactivating: {
        pageReady = false;
        selectedIp = "";
        selectedUrl = "";
        selectedSlotId = -1;
    }

    Timer {
        id: loadTimer
        interval: 320
        onTriggered: root.pageReady = true
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // ── 왼쪽 패널: 카메라 그리드 (70%) ──────────────────────────────────────
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.7
            color: Theme.bgPrimary

            CameraSplitLayout {
                anchors.fill: parent
                model: root.pageReady && typeof cameraModel !== "undefined" ? cameraModel : null
                pageType: "Device"
                selectedUrl: root.selectedUrl
                selectedSlotId: root.selectedSlotId

                onCameraSelected: (url, slotId) => {
                    root.selectedUrl = url;
                    root.selectedSlotId = slotId;

                    // IP 추출
                    root.selectedIp = (typeof deviceModel !== "undefined") ? deviceModel.deviceIp(url) : "";
                }

                onActionRequested: url => {
                // 컨텍스트 메뉴로 처리
                }

                onCameraRightClicked: (url, gx, gy) => {
                    deviceCtxMenu.targetUrl = url;
                    let mx = gx, my = gy;
                    if (mx + deviceCtxMenu.width > root.width)
                        mx = root.width - deviceCtxMenu.width - 4;
                    if (my + deviceCtxMenu.height > root.height)
                        my = root.height - deviceCtxMenu.height - 4;
                    deviceCtxMenu.x = mx;
                    deviceCtxMenu.y = my;
                    deviceCtxMenu.visible = true;
                }
            }
        }

        // ── 오른쪽 패널: Device 상세 + 제어 (30%) ─────────────────────────────
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.3
            color: Theme.bgSecondary

            // 선택 전 플레이스홀더
            Text {
                anchors.centerIn: parent
                visible: root.selectedIp === ""
                text: "Click a camera card\nto view device details."
                color: Theme.isDark ? "#555" : "#aaa"
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 13
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12
                visible: root.selectedIp !== ""

                // ── 디바이스 헤더 ───────────────────────────────────────────────
                ColumnLayout {
                    spacing: 4
                    Text {
                        text: root.selectedIp !== "" ? root.selectedIp : "No Device"
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

                // ── CPU / Memory / Temp 카드 ────────────────────────────────────
                GridLayout {
                    columns: 3
                    Layout.fillWidth: true
                    columnSpacing: 8

                    StatusCard {
                        title: "CPU"
                        value: root.selectedIp !== "" && typeof deviceModel !== "undefined" && deviceModel.hasDevice(root.selectedIp) ? deviceModel.cpu(root.selectedIp).toFixed(1) + "%" : "--"
                    }
                    StatusCard {
                        title: "Memory"
                        value: root.selectedIp !== "" && typeof deviceModel !== "undefined" && deviceModel.hasDevice(root.selectedIp) ? deviceModel.memory(root.selectedIp).toFixed(1) + "%" : "--"
                    }
                    StatusCard {
                        title: "Temp"
                        value: root.selectedIp !== "" && typeof deviceModel !== "undefined" && deviceModel.hasDevice(root.selectedIp) ? deviceModel.temp(root.selectedIp).toFixed(1) + "°C" : "--"
                    }
                }

                // ── 데이터 히스토리 그래프 ──────────────────────────────────────
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    visible: root.selectedIp !== ""

                    Text {
                        text: "History"
                        color: "#888"
                        font.pixelSize: 10
                    }

                    // Canvas 기반 그래프 (CPU)
                    DeviceGraph {
                        Layout.fillWidth: true
                        height: 60
                        label: "CPU"
                        lineColor: "#44aaff"
                        history: (root.selectedIp !== "" && typeof deviceModel !== "undefined") ? deviceModel.getHistory(root.selectedIp) : []
                        historyField: "cpu"
                    }
                    DeviceGraph {
                        Layout.fillWidth: true
                        height: 60
                        label: "Mem"
                        lineColor: "#44ff88"
                        history: (root.selectedIp !== "" && typeof deviceModel !== "undefined") ? deviceModel.getHistory(root.selectedIp) : []
                        historyField: "memory"
                    }
                    DeviceGraph {
                        Layout.fillWidth: true
                        height: 60
                        label: "Temp"
                        lineColor: "#ffaa44"
                        history: (root.selectedIp !== "" && typeof deviceModel !== "undefined") ? deviceModel.getHistory(root.selectedIp) : []
                        historyField: "temp"
                    }
                }

                // ── PTZ / IR / Heater 컨트롤 (Big Buttons) ─────────────────────
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 16
                    visible: root.selectedIp !== ""

                    Text {
                        text: qsTr("Device Controls")
                        color: Theme.fontColor
                        font.pixelSize: 16
                        font.bold: true
                    }

                    // PTZ 방향키 (Grid)
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
                                        networkBridge.sendDevice(root.selectedIp, "w", "", "");
                                }
                                onReleased: {
                                    if (typeof networkBridge !== "undefined")
                                        networkBridge.sendDevice(root.selectedIp, "", "", "");
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
                                        networkBridge.sendDevice(root.selectedIp, "a", "", "");
                                }
                                onReleased: {
                                    if (typeof networkBridge !== "undefined")
                                        networkBridge.sendDevice(root.selectedIp, "", "", "");
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
                                        networkBridge.sendDevice(root.selectedIp, "s", "", "");
                                }
                                onReleased: {
                                    if (typeof networkBridge !== "undefined")
                                        networkBridge.sendDevice(root.selectedIp, "", "", "");
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
                                        networkBridge.sendDevice(root.selectedIp, "d", "", "");
                                }
                                onReleased: {
                                    if (typeof networkBridge !== "undefined")
                                        networkBridge.sendDevice(root.selectedIp, "", "", "");
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
                            spacing: 8
                            Text {
                                text: qsTr("조도센서")
                                color: Theme.isDark ? "#aaaaaa" : "#666666"
                                font.pixelSize: 14
                            }
                            Switch {
                                id: lightSwitch
                                onToggled: {
                                    if (typeof networkBridge !== "undefined")
                                        networkBridge.sendDevice(root.selectedIp, "", checked ? "on" : "off", "");
                                }
                            }
                        }

                        ColumnLayout {
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
                                        networkBridge.sendDevice(root.selectedIp, "", "", checked ? "on" : "off");
                                }
                            }
                        }
                        ColumnLayout {
                            spacing: 8
                            Text {
                                text: qsTr("CameraAuto")
                                color: Theme.isDark ? "#aaaaaa" : "#666666"
                                font.pixelSize: 14
                            }
                            Switch {
                                id: autoSwitch
                                onToggled: {
                                    autoSendTimer.restart();
                                }
                            }
                        }
                    }
                }

                // Timer to debounce sending auto/unauto state
                Timer {
                    id: autoSendTimer
                    interval: 1000
                    repeat: false
                    onTriggered: {
                        if (typeof networkBridge !== "undefined" && root.selectedIp !== "") {
                            networkBridge.sendDevice(root.selectedIp, autoSwitch.checked ? "auto" : "unauto", "", "");
                        }
                    }
                }

                Item {
                    Layout.fillHeight: true
                }
            }
        }
    }

    // ── 주기적 히스토리 갱신 ──────────────────────────────────────────────────
    Timer {
        interval: 5000
        running: root.selectedIp !== "" && root.visible && root.pageReady
        repeat: true
        triggeredOnStart: true
        onTriggered: {
            // deviceModel.getHistory()는 항상 최신 데이터 반환
            // DeviceGraph의 history 바인딩이 자동으로 갱신되도록 selectedIp 트리거
            let ip = root.selectedIp;
            root.selectedIp = "";
            root.selectedIp = ip;
        }
    }

    // ── 상태 카드 컴포넌트 ────────────────────────────────────────────────────
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

    // ── 컨텍스트 메뉴 ─────────────────────────────────────────────────────────
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
        onDeviceControlRequested: url => {}
        onAiControlRequested: url => {}
    }
}
