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
    property int itemsPerRow: 4 // 1행당 보여줄 카드 수 (기본 4개)

    // ── newWindowArea: Outer Shell ──────────────────────────────────────────
    Rectangle {
        id: newWindowArea
        anchors.fill: parent

        // ── Layout Control ──────────────────────────────────────────────────────
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

        // ── swapWindowArea: Inner Container ─────────────────────────────────────
        Rectangle {
            id: swapWindowArea
            anchors.fill: parent
            anchors.margins: 40
            anchors.topMargin: 45 // Layout Control을 위한 여백 추가
            color: Theme.bgPrimary
            border.color: root.dragSourceIndex >= 0 ? "#5588ccff" : "#22ffffff"
            border.width: 1

            Item {
                id: gridContainer
                anchors.fill: parent
                anchors.margins: 20

                GridView {
                    id: cameraGrid
                    anchors.fill: parent

                    // 오직 가로 길이(cols 개수)에만 맞춰서 크기 계산 (비율 4:3)
                    property int cols: root.itemsPerRow
                    
                    // 여백을 고려하여 한 열에 최대로 들어갈 수 있는 너비 계산
                    property real maxW_byCol: parent.width / cols

                    // cellWidth를 가용 가능한 최대 너비로 설정 (단 최소 100)
                    cellWidth: Math.floor(Math.max(100, maxW_byCol))
                    
                    // cellWidth에서 내부 margin 20을 빼고 4:3 비율(0.75 곱하기) 적용 후 다시 margin 20 더하기
                    cellHeight: Math.floor((cellWidth - 20) * 0.75 + 20)

                    model: cameraModel
                    interactive: true // 스크롤 허용
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
                            // 동적 크기 적용 (여백 20, 가로세로 4:3)
                            width: Math.max(10, parent.width - 20)
                            height: width * 0.75

                            title: model.title
                            rtspUrl: model.rtspUrl
                            isOnline: model.isOnline
                            ctrlVisible: root.activeCtrlUrl === model.rtspUrl
                            // 드래그 중이면 원본 카드 반투명
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

                                    // 화면 밖으로 넘치면 조정 (GridView 우측/하단 여백 고려)
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
                        }

                        // ── DropArea: 다른 카드 위에 드롭 → URL 교환 ─────────────────
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
                                    cameraModel.swapCameraUrls(src, dst);
                                drag.accept(Qt.MoveAction);
                            }
                        }

                        // ── DragHandler ───────────────────────────────────────────────
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
                                            root.tryDetachWindow(delegateRoot.modelIndex, root.dragGhostX, root.dragGhostY);
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
            text: root.dragSourceIndex >= 0 ? (cameraModel.data(cameraModel.index(root.dragSourceIndex, 0), 257) ?? "") : ""
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

    function tryDetachWindow(cardIdx, spawnX, spawnY) {
        const count = windowCountMap[cardIdx] || 0;
        if (count >= 2)
            return;

        const win = cameraWindowComp.createObject(null, {
            x: Math.max(0, spawnX - 320),
            y: Math.max(0, spawnY - 240),
            width: 640,
            height: 480,
            sourceCardIndex: cardIdx
        });
        if (!win)
            return;
        const m = Object.assign({}, windowCountMap);
        m[cardIdx] = count + 1;
        windowCountMap = m;

        win.closing.connect(() => {
            const n = Object.assign({}, root.windowCountMap);
            if (n[cardIdx] > 0)
                n[cardIdx]--;
            root.windowCountMap = n;
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
    }
}
