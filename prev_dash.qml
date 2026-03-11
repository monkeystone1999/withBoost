import QtQuick
import QtQuick.Controls
import AnoMap.front

Item {
    id: root
    signal requestPage(string pageName)

    property int dragSourceIndex: -1
    property real dragGhostX: 0
    property real dragGhostY: 0
    property var windowCountMap: ({})
    property string activeCtrlUrl: ""
    property int itemsPerRow: 4 // 1?됰떦 蹂댁뿬以?移대뱶 ??(湲곕낯 4媛?

    // ?? newWindowArea: Outer Shell ??????????????????????????????????????????
    Rectangle {
        id: newWindowArea
        anchors.fill: parent

        // ?? Layout Control ??????????????????????????????????????????????????????
        Row {
            z: 10
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.topMargin: 10
            anchors.leftMargin: 40
            spacing: 10

            Text {
                text: "Grid Columns :"
                color: "white"
                font.pixelSize: 14
                anchors.verticalCenter: parent.verticalCenter
            }

            Repeater {
                model: [2, 3, 4, 5, 6]
                delegate: Rectangle {
                    width: 30
                    height: 24
                    radius: 4
                    color: root.itemsPerRow === modelData ? Theme.hanwhaFirst : "#333333"
                    border.color: "#555555"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: modelData
                        color: "white"
                        font.pixelSize: 13
                        font.bold: true
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.itemsPerRow = modelData
                    }
                }
            }
        }

        // ?? swapWindowArea: Inner Container ?????????????????????????????????????
        Rectangle {
            id: swapWindowArea
            anchors.fill: parent
            anchors.margins: 40
            anchors.topMargin: 45 // Layout Control???꾪븳 ?щ갚 異붽?
            color: Theme.bgPrimary
            border.color: root.dragSourceIndex >= 0 ? "#5588ccff" : "#22ffffff"
            border.width: 1

            // ?? Admin-Only Server Status & Pending Assigned List Overlay ??????
            Rectangle {
                id: adminPanel
                z: 200
                anchors.top: parent.top
                anchors.right: parent.right
                anchors.margins: 10
                width: 280
                height: 120
                color: "#aa000000"
                border.color: "#88ffffff"
                border.width: 1
                radius: 6
                visible: typeof loginController !== "undefined" && loginController.state === "admin"

                Column {
                    anchors.centerIn: parent
                    spacing: 8
                    Text {
                        text: qsTr("Admin Dashboard (Server Status)")
                        color: "white"
                        font.bold: true
                    }
                    Text {
                        text: typeof serverStatusModel !== "undefined" && serverStatusModel.hasServer ? qsTr("CPU: %1% | Mem: %2% | Temp: %3째C").arg(serverStatusModel.serverCpu.toFixed(1)).arg(serverStatusModel.serverMemory.toFixed(1)).arg(serverStatusModel.serverTemp.toFixed(1)) : qsTr("Server Status: Not Available")
                        color: "#aaffaa"
                        font.pixelSize: 12
                    }
                    Row {
                        spacing: 10
                        BasicButton {
                            text: "List Pending"
                            height: 30
                            onClicked: {
                                if (typeof loginController !== "undefined" && loginController.state === "admin") {
                                    if (typeof networkBridge !== "undefined") {
                                        networkBridge.sendListPending();
                                    }
                                }
                            }
                        }
                        BasicButton {
                            text: "Approve Target"
                            height: 30
                            onClicked: {
                                // For test purposes, this assumes you have a way to pick a target ID.
                                if (typeof loginController !== "undefined" && loginController.state === "admin") {
                                    if (typeof networkBridge !== "undefined") {
                                        // networkBridge.sendApprove("TARGET_ID");
                                        console.log("Approve requested");
                                    }
                                }
                            }
                        }
                    }
                }
            }

            Item {
                id: gridContainer
                anchors.fill: parent
                anchors.margins: 20

                GridView {
                    id: cameraGrid
                    anchors.fill: parent

                    // ?ㅼ쭅 媛濡?湲몄씠(cols 媛쒖닔)?먮쭔 留욎떠???ш린 怨꾩궛 (鍮꾩쑉 4:3)
                    property int cols: root.itemsPerRow

                    // ?щ갚??怨좊젮?섏뿬 ???댁뿉 理쒕?濡??ㅼ뼱媛????덈뒗 ?덈퉬 怨꾩궛
                    property real maxW_byCol: parent.width / cols

                    // cellWidth瑜?媛??媛?ν븳 理쒕? ?덈퉬濡??ㅼ젙 (??理쒖냼 100)
                    cellWidth: Math.floor(Math.max(100, maxW_byCol))

                    // cellWidth?먯꽌 ?대? margin 20??鍮쇨퀬 4:3 鍮꾩쑉(0.75 怨깊븯湲? ?곸슜 ???ㅼ떆 margin 20 ?뷀븯湲?                    cellHeight: Math.floor((cellWidth - 20) * 0.75 + 20)

                    model: cameraModel
                    interactive: true // ?ㅽ겕濡??덉슜
                    clip: true
                    reuseItems: true

                    onContentYChanged: {
                        if (root.activeCtrlUrl !== "") {
                            root.activeCtrlUrl = "";
                            overlayControlBar.visible = false;
                        }
                    }

                    delegate: Item {
                        id: delegateRoot
                        width: cameraGrid.cellWidth
                        height: cameraGrid.cellHeight
                        readonly property int modelIndex: index

                        CameraCard {
                            id: card
                            anchors.centerIn: parent
                            // ?숈쟻 ?ш린 ?곸슜 (?щ갚 20, 媛濡쒖꽭濡?4:3)
                            width: Math.max(10, parent.width - 20)
                            height: width * 0.75

                            title: model.title
                            rtspUrl: model.rtspUrl
                            isOnline: model.isOnline
                            slotId: model.slotId
                            cropRect: model.cropRect
                            ctrlVisible: root.activeCtrlUrl === model.rtspUrl
                            opacity: root.dragSourceIndex === delegateRoot.modelIndex ? 0.3 : 1.0
                            Behavior on opacity {
                                NumberAnimation {
                                    duration: 150
                                }
                            }

                            onControlRequested: {
                                if (root.activeCtrlUrl === model.rtspUrl) {
                                    root.activeCtrlUrl = "";
                                    overlayControlBar.visible = false;
                                } else {
                                    root.activeCtrlUrl = model.rtspUrl;
                                    const pos = card.mapToItem(root, 0, 0);
                                    let cx = pos.x + card.width + 8;
                                    let cy = pos.y;

                                    // ?붾㈃ 諛뽰쑝濡??섏튂硫?議곗젙 (GridView ?곗륫/?섎떒 ?щ갚 怨좊젮)
                                    if (cx + overlayControlBar.width > root.width) {
                                        cx = pos.x - overlayControlBar.width - 8;
                                    }
                                    if (cy + overlayControlBar.height > root.height) {
                                        cy = root.height - overlayControlBar.height - 8;
                                    }

                                    overlayControlBar.x = cx;
                                    overlayControlBar.y = cy;
                                    overlayControlBar.visible = true;
                                }
                            }
                            onRightClicked: (sx, sy) => {
                                overlayCtxMenu.targetUrl = model.rtspUrl;
                                overlayCtxMenu.targetSlotId = model.slotId;
                                overlayCtxMenu.targetIndex = index;
                                overlayCtxMenu.targetSplit = model.splitCount;
                                overlayCtxMenu.targetOnline = model.isOnline;
                                // sx, sy ??CameraCard ?대? 湲곗? 醫뚰몴
                                const local = root.mapFromItem(card, sx, sy);
                                let mx = local.x, my = local.y;
                                if (mx + overlayCtxMenu.width > root.width)
                                    mx = root.width - overlayCtxMenu.width - 4;
                                if (my + overlayCtxMenu.height > root.height)
                                    my = root.height - overlayCtxMenu.height - 4;
                                overlayCtxMenu.x = mx;
                                overlayCtxMenu.y = my;
                                overlayCtxMenu.visible = true;
                            }
                        }

                        // ?? DropArea: ?ㅻⅨ 移대뱶 ?꾩뿉 ?쒕∼ ??URL 援먰솚 ?????????????????
                        DropArea {
                            id: cardDrop
                            anchors.fill: parent
                            keys: ["cameraCard"]

                            Rectangle {
                                anchors.fill: parent
                                color: Theme.hanwhaFirst
                                radius: 8
                                opacity: cardDrop.containsDrag && root.dragSourceIndex !== delegateRoot.modelIndex ? 0.25 : 0.0
                                Behavior on opacity {
                                    NumberAnimation {
                                        duration: 100
                                    }
                                }
                            }

                            onDropped: drag => {
                                const src = drag.source.dragIndex;
                                const dst = delegateRoot.modelIndex;
                                if (src !== dst && src >= 0)
                                    cameraModel.swapSlots(src, dst);   // 짠3: full entry swap
                                drag.accept(Qt.MoveAction);
                            }
                        }

                        // ?? DragHandler ???????????????????????????????????????????????
                        DragHandler {
                            id: dragHandler
                            target: null
                            onCentroidChanged: {
                                if (active) {
                                    const pos = delegateRoot.mapToItem(root, centroid.position.x, centroid.position.y);
                                    root.dragGhostX = pos.x;
                                    root.dragGhostY = pos.y;
                                }
                            }
                            onActiveChanged: {
                                if (active) {
                                    root.dragSourceIndex = delegateRoot.modelIndex;
                                    const pos = delegateRoot.mapToItem(root, centroid.position.x, centroid.position.y);
                                    root.dragGhostX = pos.x;
                                    root.dragGhostY = pos.y;
                                } else {
                                    const result = ghost.Drag.drop();

                                    // Not a Swap -> Check if coordinate is outside `swapWindowArea` (margin)
                                    if (result !== Qt.MoveAction) {
                                        const swPos = root.mapFromItem(swapWindowArea, 0, 0);
                                        const isOutside = root.dragGhostX < swPos.x || root.dragGhostX > (swPos.x + swapWindowArea.width) || root.dragGhostY < swPos.y || root.dragGhostY > (swPos.y + swapWindowArea.height);

                                        if (isOutside) {
                                            root.tryDetachWindow(delegateRoot.modelIndex, model.slotId, model.title, model.rtspUrl, model.isOnline, model.cropRect, root.dragGhostX, root.dragGhostY);
                                        }
                                    }
                                    root.dragSourceIndex = -1;
                                }
                            }
                        }
                    } // delegate
                } // GridView
            } // gridContainer
        } // swapWindowArea
    } // newWindowArea

    // ?? Ghost (?쒕옒洹?以?而ㅼ꽌 異붿쟻) ???????????????????????????????????????????
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

    // ?? CameraWindow ?숈쟻 ?앹꽦 ????????????????????????????????????????????????
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

        win.closing.connect(() => {
            const n = Object.assign({}, root.windowCountMap);
            if (n[key] > 0)
                n[key]--;
            root.windowCountMap = n;
        });
    }

    // ?? Floating DeviceControlBar (Root Overlay) ??????????????????????????????
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
    }
    // ?? Context Menu (?고겢由? ???????????????????????????????????????????????????

    // ?몃? ?대┃ 罹먯튂瑜??꾪븳 蹂댁씠吏 ?딅뒗 ?꾩뿭 ?ㅻ쾭?덉씠
    MouseArea {
        anchors.fill: parent
        z: 399
        visible: overlayCtxMenu.visible
        onClicked: overlayCtxMenu.visible = false
    }

    Rectangle {
        id: overlayCtxMenu
        visible: false
        z: 400
        width: 190
        radius: 7
        color: "#1e1e2e"
        border.color: "#55aaaaff"
        border.width: 1
        height: ctxCol.implicitHeight + 12

        property string targetUrl: ""
        property int targetSlotId: -1
        property int targetIndex: -1
        property int targetSplit: 1    // model.splitCount at right-click time
        property bool targetOnline: false

        Column {
            id: ctxCol
            anchors {
                left: parent.left
                right: parent.right
                top: parent.top
                margins: 6
            }
            spacing: 2

            CtxItem {
                label: "Device ?쒖뼱"
                itemEnabled: deviceModel.hasDevice(overlayCtxMenu.targetUrl)
                onTriggered: {
                    root.activeCtrlUrl = overlayCtxMenu.targetUrl;
                    overlayControlBar.x = Math.min(overlayCtxMenu.x + overlayCtxMenu.width + 8, root.width - overlayControlBar.width - 8);
                    overlayControlBar.y = overlayCtxMenu.y;
                    overlayControlBar.visible = true;
                }
            }
            CtxItem {
                label: "AI ?쒖뼱"
                itemEnabled: true
                onTriggered: { /* AI ?쒖뼱 異뷀썑 ?곌껐 */ }
            }
            Rectangle {
                width: parent.width - 12
                height: 1
                color: "#33ffffff"
                anchors.horizontalCenter: parent.horizontalCenter
            }
            CtxItem {
                label: "Device ??뿉??蹂닿린"
                itemEnabled: deviceModel.hasDevice(overlayCtxMenu.targetUrl)
                onTriggered: root.requestPage("Device")
            }
            CtxItem {
                label: "AI ??뿉??蹂닿린"
                itemEnabled: true
                onTriggered: root.requestPage("AI")
            }
            // ?? 짠5: Split/Merge menu items ????????????????????????????????????
            Rectangle {
                width: parent.width - 12
                height: 1
                color: "#33ffffff"
                anchors.horizontalCenter: parent.horizontalCenter
            }
            CtxItem {
                label: "Split 2"
                itemEnabled: overlayCtxMenu.targetSplit === 1 && overlayCtxMenu.targetOnline
                onTriggered: cameraModel.splitSlot(overlayCtxMenu.targetIndex, 2)
            }
            CtxItem {
                label: "Split 3"
                itemEnabled: overlayCtxMenu.targetSplit === 1 && overlayCtxMenu.targetOnline
                onTriggered: cameraModel.splitSlot(overlayCtxMenu.targetIndex, 3)
            }
            CtxItem {
                label: "Split 4"
                itemEnabled: overlayCtxMenu.targetSplit === 1 && overlayCtxMenu.targetOnline
                onTriggered: cameraModel.splitSlot(overlayCtxMenu.targetIndex, 4)
            }
            CtxItem {
                label: "Merge"
                itemEnabled: overlayCtxMenu.targetSplit > 1
                onTriggered: cameraModel.mergeSlots(overlayCtxMenu.targetIndex, overlayCtxMenu.targetSplit)
            }
        }
    }

    component CtxItem: Rectangle {
        property string label: ""
        property bool itemEnabled: true
        signal triggered
        width: parent.width
        height: 32
        radius: 4
        color: itemEnabled && ma.containsMouse ? "#336688aa" : "transparent"
        Text {
            anchors {
                left: parent.left
                leftMargin: 12
                verticalCenter: parent.verticalCenter
            }
            text: parent.label
            color: parent.itemEnabled ? "white" : "#44556688"
            font.pixelSize: 12
        }
        MouseArea {
            id: ma
            anchors.fill: parent
            hoverEnabled: true
            enabled: parent.itemEnabled
            onClicked: {
                overlayCtxMenu.visible = false;
                parent.triggered();
            }
        }
    }
}
