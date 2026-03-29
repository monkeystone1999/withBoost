import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.Front
import "../layouts"
import "../Component/Camera"

Item {
    id: rootItem
    property string pageName: "AI"
    signal requestPage(string pageName)
    signal requestClose

    property string selectedCameraId: ""
    property int selectedSlotId: -1
    property string selectedIp: ""

    property bool pageReady: false
    property int currentImageIndex: -1  // wheel scroll index

    StackView.onActivating: loadTimer.start()
    StackView.onDeactivating: {
        pageReady = false;
        selectedIp = "";
        selectedCameraId = "";
        selectedSlotId = -1;
        currentImageIndex = -1;
    }

    Timer {
        id: loadTimer
        interval: 320
        onTriggered: rootItem.pageReady = true
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.7
            color: Theme.bgPrimary

            CameraSplitLayout {
                anchors.fill: parent
                model: rootItem.pageReady && typeof cameraModel !== "undefined" ? cameraModel : null
                pageType: "AI"
                selectedCameraId: rootItem.selectedCameraId
                selectedSlotId: rootItem.selectedSlotId

                onCameraSelected: (cameraId, slotId) => {
                    rootItem.selectedCameraId = cameraId;
                    rootItem.selectedSlotId = slotId;
                    rootItem.selectedIp = (typeof deviceModel !== "undefined") ? deviceModel.deviceIp(cameraId) : "";
                    rootItem.currentImageIndex = -1;  // reset to latest
                }

                onActionRequested: cameraId => {
                    console.log("AI Page: AI action requested for", cameraId);
                }

                onCameraRightClicked: (cameraId, gx, gy) => {
                    aiCtxMenu.targetCameraId = cameraId;
                    let mx = gx, my = gy;
                    if (mx + aiCtxMenu.width > rootItem.width)
                        mx = rootItem.width - aiCtxMenu.width - 4;
                    if (my + aiCtxMenu.height > rootItem.height)
                        my = rootItem.height - aiCtxMenu.height - 4;
                    aiCtxMenu.x = mx;
                    aiCtxMenu.y = my;
                    aiCtxMenu.visible = true;
                }
            }
        }

        // Right Panel: Latest AI Detection only
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: parent.width * 0.3
            color: Theme.bgSecondary

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 16
                visible: rootItem.selectedCameraId !== ""

                ColumnLayout {
                    spacing: 4
                    Text {
                        text: rootItem.selectedIp !== "" ? rootItem.selectedIp : rootItem.selectedCameraId
                        color: Theme.fontColor
                        font.pixelSize: 20
                        font.bold: true
                    }
                    Text {
                        text: "Latest AI Detection"
                        color: Theme.hanwhaFirst
                        font.pixelSize: 12
                        font.bold: true
                    }
                }

                // Image display with wheel scroll
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
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

                        property string currentTimestampText: ""

                        function updateImage() {
                            if (rootItem.selectedCameraId === "" || typeof aiImageModel === "undefined") {
                                latestAiImage.source = "";
                                latestAiImage.currentTimestampText = "";
                                imageIndexText.text = "";
                                return;
                            }
                            let events = aiImageModel.getEventsForCamera(rootItem.selectedCameraId);
                            if (!events || events.length === 0) {
                                latestAiImage.source = "";
                                latestAiImage.currentTimestampText = "";
                                imageIndexText.text = "";
                                return;
                            }
                            let idx = rootItem.currentImageIndex;
                            if (idx < 0 || idx >= events.length)
                                idx = events.length - 1;  // latest
                            rootItem.currentImageIndex = idx;
                            let evt = events[idx];
                            if (evt && evt.base64Image !== undefined && evt.base64Image !== "") {
                                latestAiImage.source = evt.base64Image;
                                imageIndexText.text = (idx + 1) + " / " + events.length;

                                // Format timestamp
                                if (evt.timestamp > 0) {
                                    let d = new Date(evt.timestamp);
                                    latestAiImage.currentTimestampText = d.toLocaleString(Qt.locale(), "yyyy-MM-dd HH:mm:ss.zzz");
                                } else {
                                    latestAiImage.currentTimestampText = "";
                                }
                            } else {
                                latestAiImage.source = "";
                                latestAiImage.currentTimestampText = "";
                                imageIndexText.text = "";
                            }
                        }

                        Component.onCompleted: updateImage()
                    }

                    // Timestamp Overlay
                    Rectangle {
                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.margins: 12
                        color: "#99000000"
                        radius: 4
                        width: timestampLabel.width + 12
                        height: timestampLabel.height + 8
                        visible: latestAiImage.currentTimestampText !== ""

                        Text {
                            id: timestampLabel
                            anchors.centerIn: parent
                            text: latestAiImage.currentTimestampText
                            color: "#ffffff"
                            font.pixelSize: 12
                            font.bold: true
                        }
                    }

                    // Wheel scroll: up=newer, down=older
                    WheelHandler {
                        onWheel: event => {
                            if (typeof aiImageModel === "undefined")
                                return;
                            let events = aiImageModel.getEventsForCamera(rootItem.selectedCameraId);
                            if (!events || events.length === 0)
                                return;
                            let idx = rootItem.currentImageIndex;
                            if (idx < 0)
                                idx = events.length - 1;
                            if (event.angleDelta.y > 0) {
                                // wheel up = newer
                                idx = Math.min(idx + 1, events.length - 1);
                            } else {
                                // wheel down = older
                                idx = Math.max(idx - 1, 0);
                            }
                            rootItem.currentImageIndex = idx;
                            latestAiImage.updateImage();
                        }
                    }

                    Connections {
                        target: typeof aiImageModel !== "undefined" ? aiImageModel : null
                        function onAiEventReceived(cameraId) {
                            if (cameraId === rootItem.selectedCameraId) {
                                rootItem.currentImageIndex = -1;  // jump to latest
                                latestAiImage.updateImage();
                            }
                        }
                    }

                    Connections {
                        target: rootItem
                        function onSelectedCameraIdChanged() {
                            rootItem.currentImageIndex = -1;
                            latestAiImage.updateImage();
                        }
                    }

                    // Index indicator
                    Text {
                        id: imageIndexText
                        anchors.bottom: parent.bottom
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.bottomMargin: 8
                        color: "#aaa"
                        font.pixelSize: 11
                        text: ""
                    }

                    Text {
                        anchors.centerIn: parent
                        text: "No Recent AI Detection\n\nScroll to browse"
                        color: Theme.isDark ? "#555" : "#aaa"
                        visible: latestAiImage.source == ""
                        horizontalAlignment: Text.AlignHCenter
                    }
                }
            }

            Text {
                anchors.centerIn: parent
                visible: rootItem.selectedCameraId === ""
                text: "Click a camera card\nto view AI detections."
                color: Theme.isDark ? "#555" : "#aaa"
                horizontalAlignment: Text.AlignHCenter
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
        onAiControlRequested: cameraId => {
            console.log("AI control requested for", cameraId);
        }
        onViewInDeviceTabRequested: cameraId => rootItem.requestPage("Device")
    }
}
