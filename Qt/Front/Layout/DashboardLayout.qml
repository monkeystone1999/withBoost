import QtQuick
import AnoMap.front
import "../Component/camera"

Item {
    id: root

    // ── 입력 Data/설정 ─────────────────────────
    property var model
    property int itemsPerRow: 4

    // ── 외부 상태 (UI 동기화용) ─────────────────
    property int dragSourceIndex: -1
    property string activeCtrlUrl: ""

    // ── Signals (Page로 위임) ───────────────────
    signal cardControlRequested(string url, real cardX, real cardY, real cardWidth, real cardHeight)
    signal cardRightClicked(int index, string url, int slotId, int splitCount, bool isOnline, real globalX, real globalY)

    signal dragStarted(int index, real globalX, real globalY)
    signal dragMoved(real globalX, real globalY)
    signal dragEndedOutside(int sourceIndex, int slotId, string title, string url, bool isOnline, rect cropRect, real globalX, real globalY)
    signal slotSwapped(int sourceIndex, int targetIndex)
    signal scrollYChanged

    GridView {
        id: cameraGrid
        anchors.fill: parent

        // 오직 가로 길이(cols 개수)에만 맞춰서 크기 계산 (비율 4:3)
        property int cols: root.itemsPerRow
        property real maxW_byCol: parent.width / cols
        cellWidth: Math.floor(Math.max(100, maxW_byCol))
        cellHeight: Math.floor((cellWidth - 20) * 0.75 + 20)

        model: root.model
        interactive: true
        clip: true
        reuseItems: true

        onContentYChanged: {
            root.scrollYChanged();
        }

        delegate: Item {
            id: delegateRoot
            width: cameraGrid.cellWidth
            height: cameraGrid.cellHeight
            readonly property int modelIndex: index

            CameraCard {
                id: card
                anchors.centerIn: parent
                width: Math.max(10, parent.width - 20)
                height: width * 0.75

                title: model.title
                rtspUrl: model.rtspUrl
                isOnline: model.isOnline
                slotId: model.slotId
                cropRect: model.cropRect

                showActionIcon: typeof deviceModel !== "undefined" && deviceModel.hasDevice(model.rtspUrl)
                highlightOnHover: root.activeCtrlUrl === model.rtspUrl
                opacity: root.dragSourceIndex === delegateRoot.modelIndex ? 0.3 : 1.0

                Behavior on opacity {
                    NumberAnimation {
                        duration: 150
                    }
                }

                function toggleControlBar() {
                    if (typeof deviceModel !== "undefined" && !deviceModel.hasDevice(model.rtspUrl))
                        return;

                    if (root.activeCtrlUrl === model.rtspUrl) {
                        root.cardControlRequested("", 0, 0, 0, 0);
                    } else {
                        // 전달: 카드 기준 좌측상단 글로벌 좌표(혹은 root 기준), 그리고 카드 크기
                        const pos = card.mapToItem(root, 0, 0);
                        root.cardControlRequested(model.rtspUrl, pos.x, pos.y, card.width, card.height);
                    }
                }

                onTapped: toggleControlBar()
                onActionIconTapped: toggleControlBar()

                onRightTapped: (sx, sy) => {
                    const local = root.mapFromItem(card, sx, sy);
                    root.cardRightClicked(index, model.rtspUrl, model.slotId, model.splitCount, model.isOnline, local.x // global 좌표계(여기서는 Layout root)로 변환
                    , local.y);
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
                        root.slotSwapped(src, dst);
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
                        root.dragMoved(pos.x, pos.y);
                    }
                }
                onActiveChanged: {
                    if (active) {
                        const pos = delegateRoot.mapToItem(root, centroid.position.x, centroid.position.y);
                        root.dragStarted(delegateRoot.modelIndex, pos.x, pos.y);
                    } else {
                        // QML Drag system에서 drop 결과를 확인하기 위해 ghost를 참조하는 로직은 Layout 외부 Page에 존재할 수 있으므로
                        // 여기서는 단순히 active 해제 신호로 처리.
                        // Wait, Ghost를 Layout 외부에서 처리하므로 Drag 액션 처리도 복잡함.
                        // Ghost가 Page에 있으니 drop() 호출은 Page의 상태에 의존.
                        // "카드 드래그 밖으로 나감" 판정 로직을 Page로 위임한다.
                        const pos = delegateRoot.mapToItem(root, centroid.position.x, centroid.position.y);
                        root.dragEndedOutside(delegateRoot.modelIndex, model.slotId, model.title, model.rtspUrl, model.isOnline, model.cropRect, pos.x, pos.y);
                    }
                }
            }
        } // delegate
    } // GridView
}
