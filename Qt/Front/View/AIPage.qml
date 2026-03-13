import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front
import "../Layout"
import "../Component/camera"

Item {
    id: root
    property string pageName: "AI"
    signal requestPage(string pageName)
    signal requestClose

    property string selectedUrl: ""
    property int selectedSlotId: -1
    property string selectedIp: ""
    property var historyData: []

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

        // ── Left Panel: Camera Grid (70%) ──────────────────────────────
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.7
            color: Theme.bgPrimary

            CameraSplitLayout {
                anchors.fill: parent
                model: root.pageReady && typeof cameraModel !== "undefined" ? cameraModel : null
                pageType: "AI"
                selectedUrl: root.selectedUrl
                selectedSlotId: root.selectedSlotId

                onCameraSelected: (url, slotId) => {
                    root.selectedUrl = url;
                    root.selectedSlotId = slotId;

                    // Extract IP from URL
                    root.selectedIp = (typeof deviceModel !== "undefined") ? deviceModel.deviceIp(url) : "";
                    console.log("AI Page: Camera selected", url, "slot", slotId, "IP", root.selectedIp);
                }

                onActionRequested: url => {
                    console.log("AI Page: AI action requested for", url);
                }

                onCameraRightClicked: (url, gx, gy) => {
                    aiCtxMenu.targetUrl = url;
                    let mx = gx, my = gy;
                    if (mx + aiCtxMenu.width > root.width)
                        mx = root.width - aiCtxMenu.width - 4;
                    if (my + aiCtxMenu.height > root.height)
                        my = root.height - aiCtxMenu.height - 4;
                    aiCtxMenu.x = mx;
                    aiCtxMenu.y = my;
                    aiCtxMenu.visible = true;
                }
            }
        }

        // ── Right Panel: Detail View (30%) ─────────────────────────────
        ColumnLayout {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.3
            spacing: 16

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: Theme.bgSecondary

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 20
                    spacing: 24
                    visible: root.selectedIp !== ""

                    // ── Header: Node Info ────────────────────────────────
                    ColumnLayout {
                        spacing: 4
                        Text {
                            text: root.selectedIp
                            color: Theme.fontColor
                            font.pixelSize: 20
                            font.bold: true
                        }
                        Text {
                            text: "AI Service Status"
                            color: Theme.hanwhaFirst
                            font.pixelSize: 12
                            font.bold: true
                        }
                    }

                    // ── Current Metrics ──────────────────────────────────
                    GridLayout {
                        columns: 3
                        Layout.fillWidth: true
                        columnSpacing: 10

                        StatusCard {
                            title: "CPU"
                            value: deviceModel.hasDevice(root.selectedIp) ? deviceModel.cpu(root.selectedIp).toFixed(1) + "%" : "--"
                        }
                        StatusCard {
                            title: "Memory"
                            value: deviceModel.hasDevice(root.selectedIp) ? deviceModel.memory(root.selectedIp).toFixed(1) + "%" : "--"
                        }
                        StatusCard {
                            title: "Temp"
                            value: deviceModel.hasDevice(root.selectedIp) ? deviceModel.temp(root.selectedIp).toFixed(1) + "°C" : "--"
                        }
                    }

                    // ── AI Control Bar ──────────────────────────────────
                    Loader {
                        Layout.fillWidth: true
                        sourceComponent: aiControlComponent
                        active: root.selectedIp !== ""
                    }

                    Component {
                        id: aiControlComponent
                        AiControlBar {
                            Layout.fillWidth: true
                            targetUrl: root.selectedUrl
                            onSendAiCmd: (url, feature, state) => {
                                console.log("AI Command Sent: url=", url, "feature=", feature, "state=", state);
                            }
                        }
                    }

                    // ── History List ─────────────────────────────────────
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 12

                        Text {
                            text: "Device History (Performance)"
                            color: Theme.fontColor
                            font.pixelSize: 14
                            font.bold: true
                        }

                        ListView {
                            id: historyList
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            spacing: 4
                            model: root.historyData

                            delegate: Rectangle {
                                width: historyList.width
                                height: 32
                                color: Theme.isDark ? "#2d2d2d" : "#f5f5f7"
                                radius: 4

                                RowLayout {
                                    anchors.fill: parent
                                    anchors.margins: 8
                                    Text {
                                        text: Qt.formatDateTime(new Date(modelData.timestamp), "hh:mm:ss")
                                        color: Theme.fontColor
                                        font.pixelSize: 11
                                        Layout.preferredWidth: 60
                                    }
                                    Text {
                                        text: "C: " + modelData.cpu.toFixed(1) + "%"
                                        color: Theme.fontColor
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                    }
                                    Text {
                                        text: "M: " + modelData.memory.toFixed(1) + "%"
                                        color: Theme.fontColor
                                        font.pixelSize: 11
                                        Layout.fillWidth: true
                                    }
                                    Text {
                                        text: modelData.temp.toFixed(1) + "°C"
                                        color: Theme.fontColor
                                        font.pixelSize: 11
                                        Layout.preferredWidth: 40
                                    }
                                }
                            }
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    visible: root.selectedIp === ""
                    text: "Click a camera card\nto configure AI features."
                    color: Theme.isDark ? "#555" : "#aaa"
                    horizontalAlignment: Text.AlignHCenter
                }
            }
        }
    }

    // ── Logic: Periodic History Fetch ────────────────────────────────────────
    Timer {
        interval: 5000
        running: !!root.selectedIp && root.visible
        repeat: true
        triggeredOnStart: true
        onTriggered: {
            root.historyData = deviceModel.getHistory(root.selectedIp);
        }
    }

    onSelectedIpChanged: {
        root.historyData = deviceModel.getHistory(root.selectedIp);
    }

    // ── Shared Component (StatusCard) ────────────────────────────────────────
    component StatusCard: Rectangle {
        property string title: ""
        property string value: ""
        Layout.fillWidth: true
        height: 60
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

    // Context Menu
    MouseArea {
        anchors.fill: parent
        z: 399
        visible: aiCtxMenu.visible
        onClicked: aiCtxMenu.visible = false
    }

    AiContextMenu {
        id: aiCtxMenu
        visible: false
        z: 400
        onAiControlRequested: url => {
            console.log("AI control requested for", url);
        }
        onViewInDeviceTabRequested: url => root.requestPage("Device")
    }
}
