import QtQuick
import QtQuick.Controls
import AnoMap.front
import "../Component/dashboard"
import "../Component/device"
import "../Layout"

Item {
    id: root
    signal requestPage(string pageName)
    signal requestClose

    property int dragSourceIndex: -1
    property real dragGhostX: 0
    property real dragGhostY: 0
    property var windowCountMap: ({})
    property string activeCtrlUrl: ""
    property int itemsPerRow: 4 // 1행당 보여줄 카드 수 (기본 4개)

    // ── newWindowArea: Outer Shell ──────────────────────────────────────────
    Rectangle {
        id: newWindowArea
        anchors.fill: parent

        // ── Layout Control ──────────────────────────────────────────────────────
        GridColumnSelector {
            z: 10
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.topMargin: 10
            anchors.leftMargin: 40
            currentValue: root.itemsPerRow
            onValueChanged: v => root.itemsPerRow = v
        }

        // ── swapWindowArea: Inner Container ─────────────────────────────────────
        Rectangle {
            id: swapWindowArea
            anchors.fill: parent
            anchors.margins: 40
            anchors.topMargin: 45 // Layout Control을 위한 여백 추가
            color: Theme.bgPrimary
            border.color: root.dragSourceIndex >= 0 ? "#5588ccff" : "#22ffffff"
            border.width: 1

            // ── Admin-Only Server Status & Pending Assigned List Overlay ──────
            AdminStatusBar {
                z: 200
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 10
                isAdmin: typeof loginController !== "undefined" && loginController.state === "admin"

                onListPendingRequested: {
                    if (typeof networkBridge !== "undefined") {
                        networkBridge.sendListPending();
                    }
                }
                onApproveTargetRequested: {
                    if (typeof networkBridge !== "undefined") {
                        // networkBridge.sendApprove("TARGET_ID");
                        console.log("Approve requested");
                    }
                }
            }

            DashboardLayout {
                id: gridLayout
                anchors.fill: parent
                anchors.margins: 20

                model: cameraModel
                itemsPerRow: root.itemsPerRow
                activeCtrlUrl: root.activeCtrlUrl
                dragSourceIndex: root.dragSourceIndex

                onCardControlRequested: (url, cardX, cardY, cardWidth, cardHeight) => {
                    if (url === "") {
                        root.activeCtrlUrl = "";
                        overlayControlBar.visible = false;
                    } else {
                        root.activeCtrlUrl = url;
                        const cardGlobalPos = root.mapFromItem(gridLayout, cardX, cardY);
                        let barX = cardGlobalPos.x + cardWidth + 8;
                        let barY = cardGlobalPos.y;

                        if (barX + overlayControlBar.width > root.width) {
                            barX = cardGlobalPos.x - overlayControlBar.width - 8;
                        }
                        if (barY + overlayControlBar.height > root.height) {
                            barY = root.height - overlayControlBar.height - 8;
                        }

                        overlayControlBar.x = barX;
                        overlayControlBar.y = barY;
                        overlayControlBar.visible = true;
                    }
                }

                onCardRightClicked: (index, url, slotId, splitCount, isOnline, globalX, globalY) => {
                    overlayCtxMenu.targetUrl = url;
                    overlayCtxMenu.targetSlotId = slotId;
                    overlayCtxMenu.targetIndex = index;
                    overlayCtxMenu.targetSplit = splitCount;
                    overlayCtxMenu.targetOnline = isOnline;

                    const pos = root.mapFromItem(gridLayout, globalX, globalY);
                    let mx = pos.x, my = pos.y;
                    if (mx + overlayCtxMenu.width > root.width)
                        mx = root.width - overlayCtxMenu.width - 4;
                    if (my + overlayCtxMenu.height > root.height)
                        my = root.height - overlayCtxMenu.height - 4;

                    overlayCtxMenu.x = mx;
                    overlayCtxMenu.y = my;
                    overlayCtxMenu.visible = true;
                }

                onDragStarted: (index, globalX, globalY) => {
                    root.dragSourceIndex = index;
                    const pos = root.mapFromItem(gridLayout, globalX, globalY);
                    root.dragGhostX = pos.x;
                    root.dragGhostY = pos.y;
                }

                onDragMoved: (globalX, globalY) => {
                    const pos = root.mapFromItem(gridLayout, globalX, globalY);
                    root.dragGhostX = pos.x;
                    root.dragGhostY = pos.y;
                }

                onDragEndedOutside: (sourceIndex, slotId, title, url, isOnline, cropRect, globalX, globalY) => {
                    ghost.Drag.drop();
                    root.dragSourceIndex = -1;
                    const pos = root.mapFromItem(gridLayout, globalX, globalY);
                    const swPos = root.mapFromItem(swapWindowArea, 0, 0);
                    const isOutside = pos.x < swPos.x || pos.x > (swPos.x + swapWindowArea.width) || pos.y < swPos.y || pos.y > (swPos.y + swapWindowArea.height);

                    if (isOutside) {
                        root.tryDetachWindow(sourceIndex, slotId, title, url, isOnline, cropRect, pos.x, pos.y);
                    }
                }

                onSlotSwapped: (sourceIndex, targetIndex) => {
                    cameraModel.swapSlots(sourceIndex, targetIndex);
                }

                onScrollYChanged: {
                    if (root.activeCtrlUrl !== "") {
                        root.activeCtrlUrl = "";
                        overlayControlBar.visible = false;
                    }
                }
            }
        } // swapWindowArea
    } // newWindowArea

    // ── Ghost (드래그 중 커서 추적) ───────────────────────────────────────────
    Rectangle {
        id: ghost
        property int dragIndex: root.dragSourceIndex
        Drag.active: root.dragSourceIndex >= 0
        Drag.source: ghost
        Drag.keys: ["cameraCard"]
        Drag.hotSpot.x: width / 2
        Drag.hotSpot.y: height / 2

        width: 320
        height: 240
        x: dragGhostX - width / 2
        y: dragGhostY - height / 2
        z: 999
        color: "#1e1e1e"
        radius: 8
        border.color: "#888888"
        border.width: 2
        opacity: root.dragSourceIndex >= 0 ? 0.7 : 0.0
        visible: opacity > 0
        Behavior on opacity {
            NumberAnimation {
                duration: 120
            }
        }
        Text {
            anchors.centerIn: parent
            text: root.dragSourceIndex >= 0 ? (cameraModel.data(cameraModel.index(root.dragSourceIndex, 0), 258) ?? "") : ""
            color: "white"
            font.pixelSize: 15
            font.bold: true
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton
        hoverEnabled: true
        propagateComposedEvents: true
        onPositionChanged: mouse => {
            if (root.dragSourceIndex < 0)
                return;
            root.dragGhostX = mouse.x;
            root.dragGhostY = mouse.y;
        }
    }

    // ── CameraWindow 동적 생성 ────────────────────────────────────────────────
    Component {
        id: cameraWindowComp
        CameraWindow {}
    }

    function tryDetachWindow(cardIdx, slotId, title, rtspUrl, isOnline, cropRect, spawnX, spawnY) {
        const key = slotId >= 0 ? slotId : cardIdx;
        const count = windowCountMap[key] || 0;
        if (count >= 2)
            return;

        const win = cameraWindowComp.createObject(null, {
            x: Math.max(0, spawnX - 320),
            y: Math.max(0, spawnY - 240),
            width: 640,
            height: 480,
            sourceSlotId: slotId,
            sourceTitle: title,
            sourceRtspUrl: rtspUrl,
            sourceOnline: isOnline,
            sourceCropRect: cropRect
        });
        if (!win)
            return;
        const m = Object.assign({}, windowCountMap);
        m[key] = count + 1;
        windowCountMap = m;

        // ▶ Main 루트 윈도우에 등록 (로그아웃 시 closeAllCameraWindows 호출에 사용)
        const mainWin = root.Window.window;
        if (mainWin && typeof mainWin.registerCameraWindow === "function") {
            mainWin.registerCameraWindow(win);
        }

        win.closing.connect(() => {
            const n = Object.assign({}, root.windowCountMap);
            if (n[key] > 0)
                n[key]--;
            root.windowCountMap = n;
            // 등록 해제
            const mw = root.Window.window;
            if (mw && typeof mw.unregisterCameraWindow === "function") {
                mw.unregisterCameraWindow(win);
            }
        });
    }

    // ── Floating DeviceControlBar (Root Overlay) ──────────────────────────────
    DeviceControlBar {
        id: overlayControlBar
        visible: false
        z: 300

        deviceIp: deviceModel.deviceIp(root.activeCtrlUrl)
        hasMotor: deviceModel.hasMotor(root.activeCtrlUrl)
        hasIr: deviceModel.hasIr(root.activeCtrlUrl)
        hasHeater: deviceModel.hasHeater(root.activeCtrlUrl)

        onCloseRequested: {
            visible = false;
            root.activeCtrlUrl = "";
        }
        onSendDeviceCmd: (ip, motor, ir, heater) => {
            if (typeof networkBridge !== "undefined" && networkBridge !== null) {
                networkBridge.sendDevice(ip, motor, ir, heater);
            }
        }
    }
    // ── Context Menu (우클릭) ───────────────────────────────────────────────────

    // 외부 클릭 캐치를 위한 보이지 않는 전역 오버레이
    MouseArea {
        anchors.fill: parent
        z: 399
        visible: overlayCtxMenu.visible
        onClicked: overlayCtxMenu.visible = false
    }

    DashboardContextMenu {
        id: overlayCtxMenu
        visible: false
        z: 400

        onDeviceControlRequested: url => {
            root.activeCtrlUrl = url;
            overlayControlBar.x = Math.min(x + width + 8, root.width - overlayControlBar.width - 8);
            overlayControlBar.y = y;
            overlayControlBar.visible = true;
        }
        onAiControlRequested: url => { /* AI 제어 추후 연결 */ }
        onViewInDeviceTabRequested: url => root.requestPage("Device")
        onViewInAiTabRequested: url => root.requestPage("AI")
        onSplitRequested: (index, n) => cameraModel.splitSlot(index, n)
        onMergeRequested: (index, currentSplit) => cameraModel.mergeSlots(index, currentSplit)
    }
}
