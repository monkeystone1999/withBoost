import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front
import "../Layout"
import "../Component/camera"
import "../Component/device"

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

                    // ── History List & Latest AI Image ─────────────────────────────────────
                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: 12

                        Text {
                            text: "Latest AI Detection"
                            color: Theme.fontColor
                            font.pixelSize: 14
                            font.bold: true
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: width * 0.75
                            color: Theme.isDark ? "#2d2d2d" : "#f5f5f7"
                            radius: 6
                            clip: true

                            Image {
                                id: latestAiImage
                                anchors.fill: parent
                                anchors.margins: 4
                                fillMode: Image.PreserveAspectFit
                                source: ""
                                visible: source !== ""

                                function updateImage() {
                                    if (root.selectedIp !== "" && typeof aiImageModel !== "undefined") {
                                        let evt = aiImageModel.getLatestEvent(root.selectedIp);
                                        if (evt && evt.base64Image !== undefined && evt.base64Image !== "") {
                                            latestAiImage.source = evt.base64Image;
                                            return;
                                        }
                                    }
                                    latestAiImage.source = "";
                                }

                                Component.onCompleted: updateImage()
                            }

                            Connections {
                                target: typeof aiImageModel !== "undefined" ? aiImageModel : null
                                function onAiEventReceived(ip) {
                                    if (ip === root.selectedIp) {
                                        latestAiImage.updateImage();
                                    }
                                }
                            }

                            Connections {
                                target: root
                                function onSelectedIpChanged() {
                                    latestAiImage.updateImage();
                                }
                            }

                            Text {
                                anchors.centerIn: parent
                                text: "No Recent AI Detection"
                                color: Theme.isDark ? "#555" : "#aaa"
                                visible: latestAiImage.source == ""
                            }
                        }

                        Text {
                            text: "Device Performance"
                            color: Theme.fontColor
                            font.pixelSize: 14
                            font.bold: true
                        }

                        LiveGraph {
                            Layout.fillWidth: true
                            height: 60
                            title: "CPU"
                            lineColor: "#44aaff"
                            targetIp: root.selectedIp
                            field: "cpu"
                        }
                        LiveGraph {
                            Layout.fillWidth: true
                            height: 60
                            title: "Mem"
                            lineColor: "#44ff88"
                            targetIp: root.selectedIp
                            field: "memory"
                        }
                        LiveGraph {
                            Layout.fillWidth: true
                            height: 60
                            title: "Temp"
                            lineColor: "#ffaa44"
                            targetIp: root.selectedIp
                            field: "temp"
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

    // (Removed manual Timer polling. Relies on LiveGraph and AiImageModel signal updates)

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
