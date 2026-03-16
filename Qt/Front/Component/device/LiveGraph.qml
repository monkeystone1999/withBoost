import QtQuick
import QtQuick.Layouts
import AnoMap.front

// ── LiveGraph — 슬라이딩 큐 기반 실시간 그래프 ─────────────────────────────
// 동작 방식:
//   - 서버/디바이스 신호가 올 때마다 최신 값 1개를 localQueue 에 push
//   - localQueue 는 최대 MAX_QUEUE 개의 값만 보관 (FIFO, 오래된 것 pop)
//   - Canvas 는 localQueue 의 전체 값을 Width 에 걸쳐 선으로 그림
//   → 새 값이 들어올 때마다 선이 오른쪽에서 자라고, 큐가 꽉 차면 좌측이 밀려남
// ─────────────────────────────────────────────────────────────────────────────
Item {
    id: root

    property string targetIp: ""
    property string field: "cpu"    // "cpu" | "memory" | "temp"
    property string title: "CPU"
    property string unit: "%"
    property color lineColor: "#44aaff"
    property bool isServer: false

    readonly property double minY: 0.0
    readonly property double maxY: 100.0
    readonly property int maxQueue: 60   // 5초×60 = 5분치

    // ── 로컬 슬라이딩 큐 ─────────────────────────────────────────────────────
    property var localQueue: []     // number[]

    // 새 값 하나를 큐에 push, 오래된 것은 pop
    function pushValue(v) {
        var q = localQueue.slice();
        q.push(typeof v === "number" ? v : Number(v) || 0);
        while (q.length > maxQueue)
            q.shift();
        localQueue = q;
        canvas.requestPaint();
    }

    // 히스토리 배열 전체를 큐에 적재 (앱 시작·타겟 변경 시 1회)
    function loadHistory(histArr) {
        if (!histArr || histArr.length === 0) {
            localQueue = [];
            canvas.requestPaint();
            return;
        }
        var q = [];
        var fn = root.field;
        for (var i = 0; i < histArr.length; i++) {
            var v = histArr[i][fn];
            q.push((v !== undefined) ? Number(v) : 0);
        }
        // 초과분 앞에서 제거
        while (q.length > maxQueue)
            q.shift();
        localQueue = q;
        canvas.requestPaint();
    }

    // 서버 신호: 최신 값 1개만 push
    function onServerUpdate() {
        if (typeof serverStatus === "undefined")
            return;
        var history = serverStatus.getServerHistory();
        if (history.length === 0)
            return;
        var latest = history[history.length - 1];
        pushValue(latest[root.field]);
    }

    // 디바이스 신호: 최신 값 1개만 push
    function onDeviceUpdate(ip) {
        if (ip !== root.targetIp || root.targetIp === "")
            return;
        if (typeof deviceModel === "undefined")
            return;
        var history = deviceModel.getHistory(root.targetIp);
        if (history.length === 0)
            return;
        var latest = history[history.length - 1];
        pushValue(latest[root.field]);
    }

    // 타겟 변경 시 전체 히스토리 재적재
    function reloadHistory() {
        if (root.isServer) {
            if (typeof serverStatus === "undefined")
                return;
            loadHistory(serverStatus.getServerHistory());
        } else {
            if (root.targetIp === "" || typeof deviceModel === "undefined") {
                localQueue = [];
                canvas.requestPaint();
                return;
            }
            loadHistory(deviceModel.getHistory(root.targetIp));
        }
    }

    // ── 신호 연결 ─────────────────────────────────────────────────────────────
    Connections {
        target: root.isServer ? (typeof serverStatus !== "undefined" ? serverStatus : null) : null
        function onStatusUpdated() {
            root.onServerUpdate();
        }
    }

    Connections {
        target: !root.isServer ? (typeof deviceModel !== "undefined" ? deviceModel : null) : null
        function onHistoryUpdated(ip) {
            root.onDeviceUpdate(ip);
        }
    }

    onTargetIpChanged: {
        localQueue = [];
        reloadHistory();
    }
    onIsServerChanged: {
        localQueue = [];
        reloadHistory();
    }
    onFieldChanged: {
        localQueue = [];
        reloadHistory();
    }
    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()

    Timer {
        id: initTimer
        interval: 500   // DevicePage lazy-load(320ms) 이후에 확실히 초기 적재
        repeat: false
        onTriggered: root.reloadHistory()
    }
    Component.onCompleted: initTimer.start()

    // ── 제목 라벨 ─────────────────────────────────────────────────────────────
    Text {
        anchors.left: parent.left
        anchors.top: parent.top
        text: root.title + " (" + root.unit + ")"
        color: "#777"
        font.pixelSize: 10
        font.bold: true
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        anchors.topMargin: 20
        antialiasing: true

        onPaint: {
            var ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);

            var values = root.localQueue;
            if (!values || values.length < 1)
                return;
            // 단 1개일 때 선을 그리려면 2개 필요
            if (values.length === 1)
                values = [values[0], values[0]];

            // ── 배경 그리드 ──────────────────────────────────────────────
            ctx.strokeStyle = "#22ffffff";
            ctx.lineWidth = 1;
            for (var g = 0; g <= 4; g++) {
                var gy = height * g / 4;
                ctx.beginPath();
                ctx.moveTo(0, gy);
                ctx.lineTo(width, gy);
                ctx.stroke();
            }

            var n = values.length;
            var range = root.maxY - root.minY;

            // ── 라인 ─────────────────────────────────────────────────────
            ctx.strokeStyle = root.lineColor;
            ctx.lineWidth = 2.0;
            ctx.lineJoin = "round";
            ctx.beginPath();
            for (var j = 0; j < n; j++) {
                var nx = width * j / Math.max(1, n - 1);
                var ny = height - (height * (values[j] - root.minY) / range);
                ny = Math.max(0, Math.min(height, ny));
                if (j === 0)
                    ctx.moveTo(nx, ny);
                else
                    ctx.lineTo(nx, ny);
            }
            ctx.stroke();

            // ── 그라데이션 채움 ──────────────────────────────────────────
            ctx.save();
            ctx.beginPath();
            for (var k = 0; k < n; k++) {
                var fx = width * k / Math.max(1, n - 1);
                var fy = height - (height * (values[k] - root.minY) / range);
                fy = Math.max(0, Math.min(height, fy));
                if (k === 0)
                    ctx.moveTo(fx, fy);
                else
                    ctx.lineTo(fx, fy);
            }
            ctx.lineTo(width, height);
            ctx.lineTo(0, height);
            ctx.closePath();

            var gradient = ctx.createLinearGradient(0, 0, 0, height);
            gradient.addColorStop(0, Qt.rgba(root.lineColor.r, root.lineColor.g, root.lineColor.b, 0.4));
            gradient.addColorStop(1, Qt.rgba(root.lineColor.r, root.lineColor.g, root.lineColor.b, 0.05));
            ctx.fillStyle = gradient;
            ctx.fill();
            ctx.restore();

            // ── 최신값 라벨 ──────────────────────────────────────────────
            var lastVal = values[n - 1];
            ctx.fillStyle = root.lineColor;
            ctx.font = "10px sans-serif";
            var textStr = lastVal.toFixed(1) + root.unit;
            var textW = ctx.measureText(textStr).width;
            ctx.fillText(textStr, width - textW - 5, 10);
        }
    }

    // 데이터 없음 오버레이
    Text {
        anchors.centerIn: canvas
        visible: root.localQueue.length === 0
        text: root.isServer ? "서버 데이터 없음\n(Admin 계정 필요)" : "데이터 없음"
        color: "#55ffffff"
        font.pixelSize: 11
        horizontalAlignment: Text.AlignHCenter
    }
}
