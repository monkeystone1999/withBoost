import QtQuick
import QtQuick.Layouts
import AnoMap.front

// ── DeviceGraph ────────────────────────────────────────────────────────────
// Canvas 기반 라인 그래프. QSG와 충돌하지 않음.
// history: deviceModel.getHistory(ip) 반환값 (QVariantList<QVariantMap>)
// historyField: "cpu" | "memory" | "temp"
// ──────────────────────────────────────────────────────────────────────────
Item {
    id: root

    property var history: []
    property string historyField: "cpu"
    property string label: "CPU"
    property color lineColor: "#44aaff"

    // 표시 범위
    readonly property double minY: 0.0
    readonly property double maxY: 100.0

    onHistoryChanged: canvas.requestPaint()
    onLineColorChanged: canvas.requestPaint()
    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()

    // 레이블
    Text {
        anchors.left: parent.left
        anchors.top: parent.top
        text: root.label
        color: "#777"
        font.pixelSize: 9
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        antialiasing: true

        onPaint: {
            var ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);

            var data = root.history;
            if (!data || data.length < 2)
                return;
            var field = root.historyField;
            var values = [];
            for (var i = 0; i < data.length; i++) {
                var v = data[i][field];
                values.push((v !== undefined) ? v : 0);
            }

            // 배경 그리드
            ctx.strokeStyle = "#22ffffff";
            ctx.lineWidth = 1;
            for (var g = 0; g <= 4; g++) {
                var gy = height * g / 4;
                ctx.beginPath();
                ctx.moveTo(0, gy);
                ctx.lineTo(width, gy);
                ctx.stroke();
            }

            // 라인 그래프
            ctx.strokeStyle = root.lineColor;
            ctx.lineWidth = 1.5;
            ctx.beginPath();
            for (var j = 0; j < values.length; j++) {
                var nx = width * j / (values.length - 1);
                var ny = height - (height * (values[j] - root.minY) / (root.maxY - root.minY));
                ny = Math.max(0, Math.min(height, ny));
                if (j === 0)
                    ctx.moveTo(nx, ny);
                else
                    ctx.lineTo(nx, ny);
            }
            ctx.stroke();

            // 마지막 값 표시
            var lastVal = values[values.length - 1];
            ctx.fillStyle = root.lineColor;
            ctx.font = "9px sans-serif";
            ctx.fillText(lastVal.toFixed(1), width - 28, 10);
        }
    }
}
