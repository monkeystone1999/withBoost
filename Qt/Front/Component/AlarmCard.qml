import QtQuick
import QtQuick.Layouts
import AnoMap.front

// 개별 알람 카드
// - 기본: 작은 토스트 (접힌 상태)
// - hover: 높이 확장 + 상세 내용 표시
// - X 버튼: hover 시 나타남, 클릭 시 제거
Rectangle {
    id: root

    // 외부에서 설정하는 알람 데이터
    property int    alarmId:   -1
    property string alarmTitle: "알람 제목"
    property string alarmDetail: "상세 내용이 여기에 표시됩니다."
    property int    severity: 0  // 0=info, 1=warning, 2=critical

    // 제거 요청 시그널
    signal dismissRequested(int id)

    // severity 색상
    readonly property color severityColor: {
        switch (severity) {
        case 2:  return "#F44336"   // critical — 빨강
        case 1:  return "#FF9800"   // warning  — 주황
        default: return "#2196F3"   // info     — 파랑
        }
    }

    // hover 상태에 따라 높이 변환
    height: hoverHandler.hovered ? expandedHeight : collapsedHeight

    readonly property int collapsedHeight: 56
    readonly property int expandedHeight:  110

    Behavior on height {
        NumberAnimation { duration: 200; easing.type: Easing.OutCubic }
    }

    width: 300
    radius: 10
    color: Theme.bgSecondary
    clip: true

    // 좌측 severity 색상 바
    Rectangle {
        width: 4
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.topMargin: 6
        anchors.bottomMargin: 6
        radius: 2
        color: root.severityColor
    }

    // hover 감지
    HoverHandler {
        id: hoverHandler
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 14
        anchors.rightMargin: 10
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        spacing: 4

        // 제목 행
        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            // severity 아이콘
            Rectangle {
                width: 8
                height: 8
                radius: 4
                color: root.severityColor
                anchors.verticalCenter: parent.verticalCenter
            }

            Text {
                text: root.alarmTitle
                font.family: Typography.fontFamilySecondary
                font.pixelSize: Typography.teriaryFont
                font.bold: true
                color: Theme.fontColor
                elide: Text.ElideRight
                Layout.fillWidth: true
            }

            // X 닫기 버튼 — hover 시에만 표시
            Rectangle {
                id: closeBtn
                width: 20
                height: 20
                radius: 10
                color: closeBtnHover.containsMouse ? "#F44336" : "transparent"
                opacity: hoverHandler.hovered ? 1.0 : 0.0

                Behavior on opacity { NumberAnimation { duration: 150 } }
                Behavior on color   { ColorAnimation  { duration: 150 } }

                Text {
                    anchors.centerIn: parent
                    text: "✕"
                    font.pixelSize: 11
                    color: closeBtnHover.containsMouse ? "white" : Theme.fontColor
                    Behavior on color { ColorAnimation { duration: 150 } }
                }

                MouseArea {
                    id: closeBtnHover
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.dismissRequested(root.alarmId)
                }
            }
        }

        // 상세 내용 — hover 시에만 opacity 1
        Text {
            text: root.alarmDetail
            font.family: Typography.fontFamilySecondary
            font.pixelSize: Typography.teriaryFont - 1
            color: Theme.fontColor
            opacity: hoverHandler.hovered ? 0.75 : 0.0
            wrapMode: Text.Wrap
            Layout.fillWidth: true
            maximumLineCount: 3
            elide: Text.ElideRight

            Behavior on opacity { NumberAnimation { duration: 180 } }
        }
    }

    // 카드 테두리
    Rectangle {
        anchors.fill: parent
        radius: parent.radius
        color: "transparent"
        border.color: Theme.isDark ? "#3A3A3C" : "#D1D1D6"
        border.width: 1
    }
}
