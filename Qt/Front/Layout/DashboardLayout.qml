import QtQuick
import AnoMap.Front
import "../Component/Camera"

Item {
    id: rootItem

    property var model
    property int itemsPerRow: 4

    property int dragSourceIndex: -1
    property string activeCtrlCameraId: ""

    signal cardControlRequested(string cameraId, real cardX, real cardY, real cardWidth, real cardHeight)
    signal cardRightClicked(int index, string cameraId, int slotId, int splitCount, bool isOnline, real globalX, real globalY)
    signal cardDoubleTapped(int index, string title, string cameraId, int slotId, bool isOnline, rect cropRect, real globalX, real globalY, real globalW, real globalH)

    signal dragStarted(int index, real globalX, real globalY)
    signal dragMoved(real globalX, real globalY)
    signal dragEndedOutside(int sourceIndex, int slotId, string title, string cameraId, bool isOnline, rect cropRect, real globalX, real globalY)
    signal slotSwapped(int sourceIndex, int targetIndex)
    signal scrollYChanged

    GridView {
        id: cameraGrid
        anchors.fill: parent

        property int cols: rootItem.itemsPerRow
        property real maxW_byCol: parent.width / cols
        cellWidth: Math.floor(Math.max(100, maxW_byCol))
        cellHeight: Math.floor((cellWidth - 20) * 0.75 + 20)

        model: rootItem.model
        interactive: true
        clip: true
        reuseItems: true

        onContentYChanged: {
            rootItem.scrollYChanged();
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
                cameraId: model.cameraId
                isOnline: model.isOnline
                slotId: model.slotId
                cropRect: model.cropRect || Qt.rect(0, 0, 1, 1)

                showActionIcon: typeof deviceModel !== "undefined" && deviceModel.hasDeviceByCameraId(model.cameraId)
                highlightOnHover: rootItem.activeCtrlCameraId === model.cameraId
                opacity: rootItem.dragSourceIndex === delegateRoot.modelIndex ? 0.3 : 1.0

                Behavior on opacity {
                    NumberAnimation {
                        duration: 150
                    }
                }

                onTapped: {
                    const pos = card.mapToItem(rootItem, 0, 0);
                    rootItem.cardDoubleTapped(index, model.title, model.cameraId, model.slotId, model.isOnline, model.cropRect || Qt.rect(0, 0, 1, 1), pos.x, pos.y, card.width, card.height);
                }
                onDoubleTapped: {
                    const pos = card.mapToItem(rootItem, 0, 0);
                    rootItem.cardDoubleTapped(index, model.title, model.cameraId, model.slotId, model.isOnline, model.cropRect || Qt.rect(0, 0, 1, 1), pos.x, pos.y, card.width, card.height);
                }
                onActionIconTapped: {
                    const pos = card.mapToItem(rootItem, 0, 0);
                    rootItem.cardDoubleTapped(index, model.title, model.cameraId, model.slotId, model.isOnline, model.cropRect || Qt.rect(0, 0, 1, 1), pos.x, pos.y, card.width, card.height);
                }

                onRightTapped: (sx, sy) => {
                    const local = rootItem.mapFromItem(card, sx, sy);
                    rootItem.cardRightClicked(index, model.cameraId, model.slotId, model.splitCount, model.isOnline, local.x, local.y);
                }
            }

            DropArea {
                id: cardDrop
                anchors.fill: parent
                keys: ["cameraCard"]

                Rectangle {
                    anchors.fill: parent
                    color: Theme.hanwhaFirst
                    radius: 8
                    opacity: cardDrop.containsDrag && rootItem.dragSourceIndex !== delegateRoot.modelIndex ? 0.25 : 0.0
                    Behavior on opacity {
                        NumberAnimation {
                            duration: 100
                        }
                    }
                }

                onDropped: drag => {
                    const src = drag.source.dragIndex;
                    const dst = delegateRoot.modelIndex;
                    if (src !== undefined && src !== dst && src >= 0) {
                        rootItem.slotSwapped(src, dst);
                        drag.accept(Qt.MoveAction);
                    }
                }
            }

            DragHandler {
                id: dragHandler
                target: null
                onCentroidChanged: {
                    if (active) {
                        const pos = delegateRoot.mapToItem(rootItem, centroid.position.x, centroid.position.y);
                        rootItem.dragMoved(pos.x, pos.y);
                    }
                }
                onActiveChanged: {
                    if (active) {
                        const pos = delegateRoot.mapToItem(rootItem, centroid.position.x, centroid.position.y);
                        rootItem.dragStarted(delegateRoot.modelIndex, pos.x, pos.y);
                    } else {
                        const pos = delegateRoot.mapToItem(rootItem, centroid.position.x, centroid.position.y);
                        rootItem.dragEndedOutside(delegateRoot.modelIndex, model.slotId, model.title, model.cameraId, model.isOnline, model.cropRect || Qt.rect(0, 0, 1, 1), pos.x, pos.y);
                    }
                }
            }
        } // delegate
    } // GridView
}
