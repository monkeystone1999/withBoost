import QtQuick
import AnoMap.back

VideoSurfaceItem {
    id: root

    property string rtspUrl: ""

    // ── worker 연결 시도 ──────────────────────────────────────────────────────
    function _tryAttach() {
        if (root.rtspUrl === "") {
            root.worker = null;
            return;
        }

        var w = videoManager.getWorker(root.rtspUrl);
        if (w) {
            root.worker = w;
        } else {
            // Worker 가 아직 없음 → 등록 완료 시그널 대기
            root.worker = null;
            videoManager.workerRegistered.connect(_onRegistered);
        }
    }

    function _onRegistered(url) {
        if (url !== root.rtspUrl)
            return;
        // 연결 해제 먼저 (중복 연결 방지)
        videoManager.workerRegistered.disconnect(_onRegistered);
        var w = videoManager.getWorker(url);
        if (w) {
            root.worker = w;
        }
    }

    onRtspUrlChanged: {
        // URL 이 바뀌면 기존 대기 연결 해제 후 다시 시도
        try { videoManager.workerRegistered.disconnect(_onRegistered); } catch(e) {}
        _tryAttach();
    }

    Component.onCompleted: {
        _tryAttach();
    }

    Component.onDestruction: {
        // 컴포넌트 소멸 시 대기 연결 정리
        try { videoManager.workerRegistered.disconnect(_onRegistered); } catch(e) {}
        root.worker = null;
    }
}
