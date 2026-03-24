import QtQuick
import QtQuick.Layouts
import AnoMap.Front

Item {
    id: rootItem

    property string targetIp: ""
    property string field: "cpu"    // "cpu" | "memory" | "temp"
    property string title: "CPU"
    property string unit: "%"
    property color lineColor: "#44aaff"
    property bool isServer: false
    property bool isCamera: false
    property string targetCameraId: ""

    readonly property double minY: 0.0
    readonly property double maxY: 100.0
    readonly property int maxQueue: 60

    property var localQueue: []     // number[]
    property bool dirty: false      // repaint throttle flag

    function pushValue(v) {
        var q = localQueue.slice();
        q.push(typeof v === "number" ? v : Number(v) || 0);
        while (q.length > maxQueue)
            q.shift();
        localQueue = q;
        dirty = true;  // mark dirty, Timer will repaint
    }

    function loadHistory(histArr) {
        if (!histArr || histArr.length === 0) {
            localQueue = [];
            canvas.requestPaint();
            return;
        }
        var q = [];
        var fn = rootItem.field;
        for (var i = 0; i < histArr.length; i++) {
            var v = histArr[i][fn];
            q.push((v !== undefined) ? Number(v) : 0);
        }
        while (q.length > maxQueue)
            q.shift();
        localQueue = q;
        canvas.requestPaint();
    }

    function onServerUpdate() {
        if (typeof serverStatus === "undefined")
            return;
        var history = serverStatus.getServerHistory();
        if (history.length === 0)
            return;
        var latest = history[history.length - 1];
        pushValue(latest[rootItem.field]);
    }

    function onDeviceUpdate(ip) {
        if (ip !== rootItem.targetIp || rootItem.targetIp === "")
            return;
        if (typeof deviceModel === "undefined")
            return;
        var history = deviceModel.getHistory(rootItem.targetIp);
        if (history.length === 0)
            return;
        var latest = history[history.length - 1];
        pushValue(latest[rootItem.field]);
    }

    function onCameraUpdate(cameraId) {
        if (cameraId !== rootItem.targetCameraId || rootItem.targetCameraId === "")
            return;
        if (typeof cameraModel === "undefined")
            return;
        var info = cameraModel.sensorInfoForCameraId(rootItem.targetCameraId);
        if (info && info[rootItem.field] !== undefined) {
            pushValue(info[rootItem.field]);
        }
    }

    function reloadHistory() {
        if (rootItem.isServer) {
            if (typeof serverStatus === "undefined")
                return;
            loadHistory(serverStatus.getServerHistory());
        } else {
            if (rootItem.targetIp === "" || typeof deviceModel === "undefined") {
                localQueue = [];
                canvas.requestPaint();
                return;
            }
            loadHistory(deviceModel.getHistory(rootItem.targetIp));
        }
    }

    // Connections removed: polling handles updates via repaintTimer

    onTargetIpChanged: {
        localQueue = [];
        canvas.requestPaint();
        if (!isCamera)
            reloadHistory();
    }
    onTargetCameraIdChanged: {
        localQueue = [];
        canvas.requestPaint();
    }
    onIsServerChanged: {
        localQueue = [];
        canvas.requestPaint();
        reloadHistory();
    }
    onFieldChanged: {
        localQueue = [];
        canvas.requestPaint();
        if (!isCamera)
            reloadHistory();
    }
    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()

    Timer {
        id: initTimer
        interval: 500
        repeat: false
        onTriggered: rootItem.reloadHistory()
    }

    // Main poll and repaint cycle: fetch latest at exactly 1Hz to eliminate CPU spikes
    Timer {
        id: repaintTimer
        interval: 1000
        repeat: true
        running: rootItem.visible
        onTriggered: {
            if (rootItem.isServer) {
                rootItem.onServerUpdate();
            } else if (rootItem.isCamera) {
                rootItem.onCameraUpdate(rootItem.targetCameraId);
            } else {
                rootItem.onDeviceUpdate(rootItem.targetIp);
            }

            if (rootItem.dirty) {
                rootItem.dirty = false;
                canvas.requestPaint();
            }
        }
    }

    Component.onCompleted: initTimer.start()

    Text {
        anchors.left: parent.left
        anchors.top: parent.top
        text: rootItem.title + " (" + rootItem.unit + ")"
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

            var values = rootItem.localQueue;
            if (!values || values.length < 1)
                return;
            if (values.length === 1)
                values = [values[0], values[0]];

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
            var range = rootItem.maxY - rootItem.minY;

            ctx.strokeStyle = rootItem.lineColor;
            ctx.lineWidth = 2.0;
            ctx.lineJoin = "round";
            ctx.beginPath();
            for (var j = 0; j < n; j++) {
                var nx = width * j / Math.max(1, n - 1);
                var ny = height - (height * (values[j] - rootItem.minY) / range);
                ny = Math.max(0, Math.min(height, ny));
                if (j === 0)
                    ctx.moveTo(nx, ny);
                else
                    ctx.lineTo(nx, ny);
            }
            ctx.stroke();

            ctx.save();
            ctx.beginPath();
            for (var k = 0; k < n; k++) {
                var fx = width * k / Math.max(1, n - 1);
                var fy = height - (height * (values[k] - rootItem.minY) / range);
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
            gradient.addColorStop(0, Qt.rgba(rootItem.lineColor.r, rootItem.lineColor.g, rootItem.lineColor.b, 0.4));
            gradient.addColorStop(1, Qt.rgba(rootItem.lineColor.r, rootItem.lineColor.g, rootItem.lineColor.b, 0.05));
            ctx.fillStyle = gradient;
            ctx.fill();
            ctx.restore();

            var lastVal = values[n - 1];
            ctx.fillStyle = rootItem.lineColor;
            ctx.font = "10px sans-serif";
            var textStr = lastVal.toFixed(1) + rootItem.unit;
            var textW = ctx.measureText(textStr).width;
            ctx.fillText(textStr, width - textW - 5, 10);
        }
    }

    Text {
        anchors.centerIn: canvas
        visible: rootItem.localQueue.length === 0
        text: rootItem.isServer ? qsTr("서버 데이터 로드 중...") : qsTr("표시할 데이터가 없습니다")
        color: "#55ffffff"
        font.pixelSize: 11
        horizontalAlignment: Text.AlignHCenter
    }
}
