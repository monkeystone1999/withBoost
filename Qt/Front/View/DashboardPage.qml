import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front
import AnoMap.back

Item {
    id: root

    signal requestPage(string pageName)

    // ── 드래그 상태 추적 ─────────────────────────────────────────────────────
    // 현재 드래그 중인 카드의 모델 인덱스 (-1 = 없음)
    property int  dragSourceIndex: -1
    property real dragGhostX: 0
    property real dragGhostY: 0

    // 카드당 생성된 CameraWindow 수 추적 { index: count }
    property var windowCountMap: ({})

    Rectangle {
        anchors.fill: parent
        color: Theme.bgPrimary

        // ── 그리드 ────────────────────────────────────────────────────────────
        GridView {
            id: cameraGrid
            anchors.fill: parent
            anchors.margins: 20

            cellWidth:  340   // 320 + 20 여백
            cellHeight: 260   // 240 + 20 여백

            model: cameraModel

            clip: true

            delegate: Item {
                id: delegateRoot
                width:  cameraGrid.cellWidth
                height: cameraGrid.cellHeight

                // 모델 인덱스 캡처
                readonly property int modelIndex: index

                // ── 카드 ──────────────────────────────────────────────────────
                CameraCard {
                    id: card
                    anchors.centerIn: parent
                    title:    model.title
                    rtspUrl:  model.rtspUrl
                    isOnline: model.isOnline

                    // 현재 이 카드가 drag 소스이면 반투명
                    opacity: root.dragSourceIndex === delegateRoot.modelIndex ? 0.3 : 1.0
                    Behavior on opacity { NumberAnimation { duration: 150 } }
                }

                // ── DropArea: 다른 카드를 이 위에 드랍하면 URL 교환 ──────────
                DropArea {
                    id: dropArea
                    anchors.fill: parent
                    keys: ["cameraCard"]

                    // hover 표시
                    Rectangle {
                        anchors.fill: parent
                        color: Theme.hanwhaFirst
                        opacity: dropArea.containsDrag
                                 && root.dragSourceIndex !== delegateRoot.modelIndex
                                 ? 0.25 : 0.0
                        radius: 8
                        Behavior on opacity { NumberAnimation { duration: 100 } }
                    }

                    onDropped: (drag) => {
                        var srcIdx = drag.source.dragIndex
                        var dstIdx = delegateRoot.modelIndex
                        if (srcIdx !== dstIdx && srcIdx >= 0) {
                            cameraModel.swapCameraUrls(srcIdx, dstIdx)
                        }
                        root.dragSourceIndex = -1
                    }
                }

                // ── DragArea: 카드를 잡아서 드래그 ──────────────────────────
                Item {
                    id: dragSource
                    anchors.fill: parent

                    // DropArea가 인식하는 drag.source
                    property int dragIndex: delegateRoot.modelIndex
                    Drag.active:   dragHandler.active
                    Drag.keys:     ["cameraCard"]
                    Drag.hotSpot.x: width  / 2
                    Drag.hotSpot.y: height / 2

                    DragHandler {
                        id: dragHandler
                        onActiveChanged: {
                            if (active) {
                                root.dragSourceIndex = delegateRoot.modelIndex
                                dragSource.Drag.start()
                            } else {
                                // 드랍 대상이 없거나 grid 밖에 드랍 → detach 판정
                                var accepted = dragSource.Drag.drop()
                                if (accepted !== Qt.MoveAction) {
                                    // grid 밖 → CameraWindow 생성 시도
                                    root.tryDetachWindow(
                                        delegateRoot.modelIndex,
                                        dragHandler.centroid.scenePosition.x,
                                        dragHandler.centroid.scenePosition.y
                                    )
                                }
                                root.dragSourceIndex = -1
                            }
                        }
                    }
                }
            } // delegate
        } // GridView
    }

    // ── Ghost 이미지 (드래그 중 커서 따라다니는 반투명 카드 preview) ─────────
    CameraCard {
        id: ghost
        width:  320
        height: 240
        x: dragGhostX - width  / 2
        y: dragGhostY - height / 2
        z: 999
        opacity: root.dragSourceIndex >= 0 ? 0.75 : 0.0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 120 } }

        // 드래그 중인 카드의 데이터를 표시
        title:    root.dragSourceIndex >= 0
                  ? cameraModel.data(cameraModel.index(root.dragSourceIndex, 0), 257)
                  : ""  // TitleRole = Qt.UserRole+1 = 257 는 QML에서 직접 접근 불가이므로 숨김
        rtspUrl:  ""
        isOnline: false

        // 실제로는 title 표시만으로도 충분 — 복잡한 video 렌더링 불필요
        Component.onCompleted: {
            // ghost는 video stream 없이 단순 외양만
        }
    }

    // 마우스 이동 시 ghost 위치 업데이트
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.NoButton  // 클릭은 DragHandler에 위임
        hoverEnabled: true
        propagateComposedEvents: true
        onPositionChanged: (mouse) => {
            root.dragGhostX = mouse.x
            root.dragGhostY = mouse.y
        }
    }

    // ── CameraWindow 컴포넌트 (동적 생성용) ─────────────────────────────────
    Component {
        id: cameraWindowComponent
        CameraWindow {}
    }

    // ── Detach 처리 함수 ─────────────────────────────────────────────────────
    function tryDetachWindow(cardIndex, spawnX, spawnY) {
        var count = windowCountMap[cardIndex] || 0
        if (count >= 2) {
            console.log("CameraWindow limit reached for card", cardIndex)
            return
        }

        var title    = cameraModel.data(cameraModel.index(cardIndex, 0), 257) || ("Camera " + cardIndex)
        var rtspUrl  = cameraModel.data(cameraModel.index(cardIndex, 0), 258) || ""
        var isOnline = cameraModel.data(cameraModel.index(cardIndex, 0), 259) || false

        var win = cameraWindowComponent.createObject(null, {
            title:    title,
            rtspUrl:  rtspUrl,
            isOnline: isOnline,
            x: spawnX - 200,
            y: spawnY - 150,
            width:  640,
            height: 480,
            sourceCardIndex: cardIndex
        })

        if (win) {
            var newMap = Object.assign({}, windowCountMap)
            newMap[cardIndex] = count + 1
            windowCountMap = newMap

            // 창 닫힘 시 카운트 감소
            win.closing.connect(function() {
                var m = Object.assign({}, root.windowCountMap)
                if (m[cardIndex] > 0) m[cardIndex]--
                root.windowCountMap = m
            })
        }
    }
}
