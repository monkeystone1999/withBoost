import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.Front
import "../Component/Camera"
import "../Component/Device"
import "../Component/Dashboard"

Item {
    id: rootItem
    signal requestPage(string pageName)
    signal requestClose

    property string activeCtrlCameraId: ""
    property int dragSourceIndex: -1

    property bool pageReady: false
    property int gridColumns: typeof settingsController !== "undefined" ? settingsController.defaultGrid : 4
    StackView.onActivating: pageLoadTimer.start()
    StackView.onDeactivating: pageReady = false
    Timer {
        id: pageLoadTimer
        interval: 320
        onTriggered: rootItem.pageReady = true
    }

    Rectangle {
        id: mainArea
        anchors.fill: parent
        color: Theme.bgPrimary

        DashboardLayout {
            id: gridLayout
            anchors.fill: parent
            anchors.margins: 20
            model: rootItem.pageReady && typeof cameraModel !== "undefined" ? cameraModel : null
            itemsPerRow: rootItem.gridColumns

            dragSourceIndex: rootItem.dragSourceIndex
            activeCtrlCameraId: rootItem.activeCtrlCameraId

            onCardControlRequested: (cameraId, cardX, cardY, cardWidth, cardHeight) => {}

            onCardRightClicked: (index, cameraId, slotId, splitCount, isOnline, globalX, globalY) => {
                cameraContextMenu.targetIndex = index;
                cameraContextMenu.targetCameraId = cameraId;
                cameraContextMenu.targetSlotId = slotId;
                cameraContextMenu.targetSplit = splitCount;
                cameraContextMenu.targetOnline = isOnline;

                let mx = globalX, my = globalY;
                if (mx + cameraContextMenu.width > rootItem.width)
                    mx = rootItem.width - cameraContextMenu.width - 4;
                if (my + cameraContextMenu.height > rootItem.height)
                    my = rootItem.height - cameraContextMenu.height - 4;

                cameraContextMenu.x = mx;
                cameraContextMenu.y = my;
                cameraContextMenu.visible = true;
            }

            onDragStarted: (index, globalX, globalY) => {
                rootItem.dragSourceIndex = index;
                ghost.dragIndex = index;
                ghost.x = globalX - ghost.width / 2;
                ghost.y = globalY - ghost.height / 2;
                ghost.visible = true;
                ghost.Drag.active = true;
            }

            onDragMoved: (globalX, globalY) => {
                ghost.x = globalX - ghost.width / 2;
                ghost.y = globalY - ghost.height / 2;
            }

            onDragEndedOutside: (sourceIndex, slotId, title, cameraId, isOnline, cropRect, globalX, globalY) => {
                ghost.Drag.drop();
                ghost.Drag.active = false;
                ghost.visible = false;
                rootItem.dragSourceIndex = -1;

                const pos = rootItem.mapFromItem(gridLayout, globalX, globalY);
                const swPos = rootItem.mapFromItem(swapWindowArea, 0, 0);
                const isOutside = pos.x < swPos.x || pos.x > (swPos.x + swapWindowArea.width) || pos.y < swPos.y || pos.y > (swPos.y + swapWindowArea.height);
                if (isOutside) {
                    if (typeof appController !== "undefined") {
                        appController.detachCameraWindow(slotId, title, cameraId, isOnline, cropRect, pos.x, pos.y);
                    }
                }
            }

            onSlotSwapped: (sourceIndex, targetIndex) => {
                if (typeof cameraModel !== "undefined") {
                    cameraModel.swapSlots(sourceIndex, targetIndex);
                }
            }

            onScrollYChanged: {
                rootItem.activeCtrlCameraId = "";
            }

            onCardDoubleTapped: (index, title, cameraId, slotId, isOnline, cropRect, globalX, globalY, w, h) => {
                const pos = rootItem.mapFromItem(gridLayout, globalX, globalY);
                fullScreenOverlay.openDialog(title, cameraId, slotId, isOnline, cropRect, pos.x, pos.y, w, h);
            }
        }

        Rectangle {
            id: ghost
            visible: false
            z: 500
            width: 240
            height: 180
            color: Theme.bgSecondary
            radius: 8
            border.color: Theme.hanwhaFirst
            border.width: 2
            opacity: 0.8

            property int dragIndex: -1

            Drag.active: false
            Drag.source: ghost
            Drag.keys: ["cameraCard"]

            Text {
                anchors.centerIn: parent
                text: "Dragging..."
                color: Theme.fontColor
                font.pixelSize: 14
            }
        }

        Rectangle {
            id: swapWindowArea
            anchors.fill: gridLayout
            color: "transparent"
            z: -1
        }
        Rectangle {
            id: gridFab
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            anchors.margins: 20
            width: 48
            height: 48
            radius: 24
            color: Theme.hanwhaFirst
            z: 10

            Text {
                anchors.centerIn: parent
                text: rootItem.gridColumns + "x"
                color: "white"
                font.pixelSize: 16
                font.bold: true
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: gridMenu.popup()
            }

            Menu {
                id: gridMenu
                Repeater {
                    model: [2, 3, 4, 5, 6]
                    MenuItem {
                        text: modelData + " Columns"
                        onTriggered: {
                            rootItem.gridColumns = modelData;
                            if (typeof settingsController !== "undefined") {
                                settingsController.defaultGrid = modelData;
                                settingsController.save();
                            }
                        }
                    }
                }
            }
        }
    }
    MouseArea {
        anchors.fill: parent
        z: 400
        visible: cameraContextMenu.visible
        onClicked: {
            cameraContextMenu.visible = false;
        }
    }

    DashboardContextMenu {
        id: cameraContextMenu
        visible: false
        z: 500

        onSplitRequested: (index, n, direction) => {
            if (typeof cameraModel !== "undefined") {
                cameraModel.splitSlot(index, n, direction);
            }
            cameraContextMenu.visible = false;
        }

        onMergeRequested: (index, currentSplit) => {
            if (typeof cameraModel !== "undefined") {
                cameraModel.mergeSlots(index, currentSplit);
            }
            cameraContextMenu.visible = false;
        }

        onDeviceControlRequested: cameraId => {
            rootItem.activeCtrlCameraId = cameraId;
            cameraContextMenu.visible = false;
        }

        onAiControlRequested: cameraId => {
            cameraContextMenu.visible = false;
        }

        onViewInDeviceTabRequested: cameraId => {
            rootItem.requestPage("Device");
            cameraContextMenu.visible = false;
        }

        onViewInAiTabRequested: cameraId => {
            rootItem.requestPage("AI");
            cameraContextMenu.visible = false;
        }
    }

    Rectangle {
        id: fullScreenOverlay
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.8)
        visible: false
        z: 1000
        opacity: 0

        property string targetTitle: ""
        property string targetCameraId: ""
        property int targetSlotId: -1
        property bool isOnline: false
        property rect cropRect: Qt.rect(0, 0, 1, 1)
        property real startX: 0
        property real startY: 0
        property real startW: 0
        property real startH: 0
        property real zoomScale: 1.0
        property real panX: 0
        property real panY: 0

        Behavior on opacity {
            NumberAnimation {
                duration: 300
            }
        }

        MouseArea {
            anchors.fill: parent
            onClicked: fullScreenOverlay.closeDialog()
        }

        Item {
            id: animContainer
            x: fullScreenOverlay.startX
            y: fullScreenOverlay.startY
            width: fullScreenOverlay.startW
            height: fullScreenOverlay.startH

            Behavior on x {
                NumberAnimation {
                    duration: 300
                    easing.type: Easing.OutCubic
                }
            }
            Behavior on y {
                NumberAnimation {
                    duration: 300
                    easing.type: Easing.OutCubic
                }
            }
            Behavior on width {
                NumberAnimation {
                    duration: 300
                    easing.type: Easing.OutCubic
                }
            }
            Behavior on height {
                NumberAnimation {
                    duration: 300
                    easing.type: Easing.OutCubic
                }
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                // Title Bar (Fixed)
                Rectangle {
                    Layout.fillWidth: true
                    height: 32
                    color: "#2d2d2d"
                    z: 10
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 12
                        anchors.rightMargin: 12
                        Text {
                            text: fullScreenOverlay.targetTitle
                            color: "white"
                            font.pixelSize: 14
                            font.bold: true
                            Layout.fillWidth: true
                            elide: Text.ElideRight
                        }
                        Rectangle {
                            width: 10
                            height: 10
                            radius: 5
                            color: fullScreenOverlay.isOnline ? "#4caf50" : "#f44336"
                        }
                    }
                }

                // Zoomable Video Area
                Item {
                    id: videoContainer
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true

                    Item {
                        id: zoomContent
                        width: videoContainer.width
                        height: videoContainer.height
                        scale: fullScreenOverlay.zoomScale
                        x: fullScreenOverlay.panX
                        y: fullScreenOverlay.panY
                        transformOrigin: Item.TopLeft

                        Loader {
                            id: lazyLoader
                            anchors.fill: parent
                            active: false
                            sourceComponent: VideoSurface {
                                slotId: fullScreenOverlay.targetSlotId
                                cameraId: fullScreenOverlay.targetCameraId
                                cropRect: fullScreenOverlay.cropRect
                            }
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onWheel: wheel => {
                            let zoomFactor = wheel.angleDelta.y > 0 ? 1.1 : 0.9;
                            let oldScale = fullScreenOverlay.zoomScale;
                            let newScale = oldScale * zoomFactor;
                            if (newScale < 1.0)
                                newScale = 1.0;
                            if (newScale > 3.0)
                                newScale = 3.0;

                            if (newScale === oldScale)
                                return;

                            // Zoom at mouse point (mx, my are in videoContainer space)
                            let mx = wheel.x;
                            let my = wheel.y;
                            let cx = (mx - fullScreenOverlay.panX) / oldScale;
                            let cy = (my - fullScreenOverlay.panY) / oldScale;

                            fullScreenOverlay.zoomScale = newScale;
                            if (newScale <= 1.0) {
                                fullScreenOverlay.panX = 0;
                                fullScreenOverlay.panY = 0;
                            } else {
                                fullScreenOverlay.panX = mx - cx * newScale;
                                fullScreenOverlay.panY = my - cy * newScale;

                                // Constrain pan
                                let minX = videoContainer.width * (1 - newScale);
                                let minY = videoContainer.height * (1 - newScale);
                                if (fullScreenOverlay.panX > 0)
                                    fullScreenOverlay.panX = 0;
                                if (fullScreenOverlay.panX < minX)
                                    fullScreenOverlay.panX = minX;
                                if (fullScreenOverlay.panY > 0)
                                    fullScreenOverlay.panY = 0;
                                if (fullScreenOverlay.panY < minY)
                                    fullScreenOverlay.panY = minY;
                            }
                        }

                        onClicked: mouse => {
                            let dist = Math.sqrt(Math.pow(mouse.x - lastPos.x, 2) + Math.pow(mouse.y - lastPos.y, 2));
                            if (dist < 5) {
                                fullScreenOverlay.closeDialog();
                            }
                        }

                        property point lastPos
                        onPressed: mouse => lastPos = Qt.point(mouse.x, mouse.y)
                        onPositionChanged: mouse => {
                            if (pressed && fullScreenOverlay.zoomScale > 1.0) {
                                let dx = mouse.x - lastPos.x;
                                let dy = mouse.y - lastPos.y;

                                // Apply movement to current pan
                                let newX = fullScreenOverlay.panX + dx;
                                let newY = fullScreenOverlay.panY + dy;

                                // Constrain so video always fills the container
                                let minX = videoContainer.width * (1 - fullScreenOverlay.zoomScale);
                                let minY = videoContainer.height * (1 - fullScreenOverlay.zoomScale);

                                fullScreenOverlay.panX = Math.min(0, Math.max(minX, newX));
                                fullScreenOverlay.panY = Math.min(0, Math.max(minY, newY));

                                lastPos = Qt.point(mouse.x, mouse.y);
                            }
                        }
                    }
                }
            }
        }

        Timer {
            id: animTimer
            interval: 300
            onTriggered: {
                if (fullScreenOverlay.opacity > 0) {
                    lazyLoader.active = true;
                } else {
                    fullScreenOverlay.visible = false;
                    lazyLoader.active = false;
                }
            }
        }

        function openDialog(title, cameraId, slotId, online, crop, sx, sy, sw, sh) {
            fullScreenOverlay.targetTitle = title;
            fullScreenOverlay.targetCameraId = cameraId;
            fullScreenOverlay.targetSlotId = slotId;
            fullScreenOverlay.isOnline = online;
            fullScreenOverlay.cropRect = crop;

            fullScreenOverlay.zoomScale = 1.0;
            fullScreenOverlay.panX = 0;
            fullScreenOverlay.panY = 0;

            fullScreenOverlay.startX = sx;
            fullScreenOverlay.startY = sy;
            fullScreenOverlay.startW = sw;
            fullScreenOverlay.startH = sh;

            animContainer.x = sx;
            animContainer.y = sy;
            animContainer.width = sw;
            animContainer.height = sh;

            fullScreenOverlay.visible = true;
            fullScreenOverlay.opacity = 1;

            Qt.callLater(() => {
                animContainer.x = 10;
                animContainer.y = 10;
                animContainer.width = fullScreenOverlay.width - 20;
                animContainer.height = fullScreenOverlay.height - 20;
            });
            animTimer.start();
        }

        function closeDialog() {
            lazyLoader.active = false;
            animContainer.x = fullScreenOverlay.startX;
            animContainer.y = fullScreenOverlay.startY;
            animContainer.width = fullScreenOverlay.startW;
            animContainer.height = fullScreenOverlay.startH;
            fullScreenOverlay.opacity = 0;
            animTimer.start();
        }
    }
}
