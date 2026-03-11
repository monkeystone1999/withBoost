import QtQuick
import AnoMap.back

VideoSurfaceItem {
    id: root

    // §3: lookup by slotId so that card reorder (swap) does NOT cause reconnect
    property int slotId: -1
    // Legacy fallback — used when slotId is not set
    property string rtspUrl: ""

    // §4: cropRect passed from CameraModel (tile UV coordinates)
    // Default (0,0,1,1) = full frame; tile entries carry e.g. (0,0,0.5,0.5)
    // Bound in CameraCard.qml from model.cropRect

    // ── worker connection ─────────────────────────────────────────────────────
    function _tryAttach() {
        var w = null;
        if (root.slotId >= 0) {
            w = videoManager.getWorkerBySlot(root.slotId);
            if (!w && root.rtspUrl !== "") {
                w = videoManager.getWorker(root.rtspUrl);
            }
        } else if (root.rtspUrl !== "") {
            w = videoManager.getWorker(root.rtspUrl);
        }
        if (w) {
            root.worker = w;
        } else {
            root.worker = null;
            try {
                videoManager.workerRegistered.disconnect(_onRegistered);
            } catch (e) {}
            videoManager.workerRegistered.connect(_onRegistered);
        }
    }

    function _onRegistered(url) {
        const isTargetUrl = (root.slotId >= 0) ? (videoManager.slotUrl(root.slotId) === url) : (root.rtspUrl === url);
        if (isTargetUrl) {
            try {
                videoManager.workerRegistered.disconnect(_onRegistered);
            } catch (e) {}
            _tryAttach();
        }
    }

    onSlotIdChanged: {
        try {
            videoManager.workerRegistered.disconnect(_onRegistered);
        } catch (e) {}
        _tryAttach();
    }

    onRtspUrlChanged: {
        try {
            videoManager.workerRegistered.disconnect(_onRegistered);
        } catch (e) {}
        _tryAttach();
    }

    // QTimer(33ms) 가 VideoSurfaceItem 내부에서 update() 를 구동함
    // 외부 시그널 연결 불필요

    Component.onCompleted: _tryAttach()

    Component.onDestruction: {
        try {
            videoManager.workerRegistered.disconnect(_onRegistered);
        } catch (e) {}
        root.worker = null;
    }
}
