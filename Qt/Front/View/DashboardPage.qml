import QtQuick
import QtQuick.Controls
import AnoMap.front
import "../Component/camera"
import "../Component/device"
import "../Component/dashboard"

// ══════════════════════════════════════════════════════════════════════════════
// DashboardPage — Camera Grid View (All Users)
// User들을 위한 Camera 모니터링 페이지
// Admin 전용 User 관리는 AdminPage.qml 참조
// ══════════════════════════════════════════════════════════════════════════════

Item {
    id: root
    signal requestPage(string pageName)
    signal requestClose

    property string activeCtrlUrl: ""
    property int dragSourceIndex: -1

    Rectangle {
        id: mainArea
        anchors.fill: parent
        color: Theme.bgPrimary

        // ── Camera Grid Layout ─────────────────────────────────────────────
        DashboardLayout {
            id: gridLayout
            anchors.fill: parent
            anchors.margins: 20
            model: typeof cameraModel !== "undefined" ? cameraModel : null
            itemsPerRow: 4

            dragSourceIndex: root.dragSourceIndex
            activeCtrlUrl: root.activeCtrlUrl

            onCardControlRequested: (url, cardX, cardY, cardWidth, cardHeight) => {
                if (url === "") {
                    root.activeCtrlUrl = "";
                    controlBar.visible = false;
                    return;
                }
                root.activeCtrlUrl = url;
                controlBar.visible = true;
                controlBar.x = cardX;
                controlBar.y = cardY + cardHeight + 8;
            }

            onCardRightClicked: (index, url, slotId, splitCount, isOnline, globalX, globalY) => {
                cameraContextMenu.targetIndex = index;
                cameraContextMenu.targetUrl = url;
                cameraContextMenu.targetSlotId = slotId;
                cameraContextMenu.targetSplit = splitCount;
                cameraContextMenu.targetOnline = isOnline;

                let mx = globalX, my = globalY;
                if (mx + cameraContextMenu.width > root.width)
                    mx = root.width - cameraContextMenu.width - 4;
                if (my + cameraContextMenu.height > root.height)
                    my = root.height - cameraContextMenu.height - 4;

                cameraContextMenu.x = mx;
                cameraContextMenu.y = my;
                cameraContextMenu.visible = true;
            }

            onDragStarted: (index, globalX, globalY) => {
                root.dragSourceIndex = index;
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

            onDragEndedOutside: (sourceIndex, slotId, title, url, isOnline, cropRect, globalX, globalY) => {
                ghost.Drag.drop();
                ghost.Drag.active = false;
                ghost.visible = false;
                root.dragSourceIndex = -1;

                const pos = root.mapFromItem(gridLayout, globalX, globalY);
                const swPos = root.mapFromItem(swapWindowArea, 0, 0);
                const isOutside = pos.x < swPos.x || pos.x > (swPos.x + swapWindowArea.width) || pos.y < swPos.y || pos.y > (swPos.y + swapWindowArea.height);
                if (isOutside) {
                    root.tryDetachWindow(sourceIndex, slotId, title, url, isOnline, cropRect, pos.x, pos.y);
                }
            }

            onSlotSwapped: (sourceIndex, targetIndex) => {
                if (typeof cameraModel !== "undefined") {
                    cameraModel.swapSlots(sourceIndex, targetIndex);
                }
            }

            onScrollYChanged: {
                controlBar.visible = false;
                root.activeCtrlUrl = "";
            }
        }

        // ── Drag Ghost ─────────────────────────────────────────────────────
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

        // ── Control Bar ───────────────────────────────────────────────────
        DeviceControlBar {
            id: controlBar
            visible: false
            z: 300

            deviceIp: typeof deviceModel !== "undefined" ? deviceModel.deviceIp(root.activeCtrlUrl) : ""
            hasMotor: typeof deviceModel !== "undefined" ? deviceModel.hasMotor(root.activeCtrlUrl) : false
            hasIr: typeof deviceModel !== "undefined" ? deviceModel.hasIr(root.activeCtrlUrl) : false
            hasHeater: typeof deviceModel !== "undefined" ? deviceModel.hasHeater(root.activeCtrlUrl) : false

            onCloseRequested: {
                root.activeCtrlUrl = "";
                controlBar.visible = false;
            }

            onSendDeviceCmd: (ip, motor, ir, heater) => {
                if (typeof networkBridge !== "undefined") {
                    networkBridge.sendDevice(ip, motor, ir, heater);
                }
            }
        }

        // ── Swap Window Drop Area ─────────────────────────────────────────
        Rectangle {
            id: swapWindowArea
            anchors.fill: gridLayout
            color: "transparent"
            z: -1
        }
    }

    // ── Camera Context Menu ────────────────────────────────────────────────
    MouseArea {
        anchors.fill: parent
        z: 499
        visible: cameraContextMenu.visible
        onClicked: cameraContextMenu.visible = false
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

        onDeviceControlRequested: url => {
            root.activeCtrlUrl = url;
            controlBar.visible = true; // visibility is managed via activeCtrlUrl but we force it here
            cameraContextMenu.visible = false;
        }

        onAiControlRequested: url => {
            cameraContextMenu.visible = false;
        }

        onViewInDeviceTabRequested: url => {
            root.requestPage("Device");
            cameraContextMenu.visible = false;
        }

        onViewInAiTabRequested: url => {
            root.requestPage("AI");
            cameraContextMenu.visible = false;
        }
    }

    // ── Detach Window Helper ───────────────────────────────────────────────
    function tryDetachWindow(sourceIndex, slotId, title, url, isOnline, cropRect, globalX, globalY) {
        const comp = Qt.createComponent("../Component/camera/CameraWindow.qml");
        if (comp.status === Component.Ready) {
            const win = comp.createObject(root, {
                transientParent: root.Window.window,
                sourceSlotId: slotId,
                sourceTitle: title,
                sourceRtspUrl: url,
                sourceOnline: isOnline,
                sourceCropRect: cropRect
            });
            win.x = globalX;
            win.y = globalY;
            win.show();
        }
    }
}
