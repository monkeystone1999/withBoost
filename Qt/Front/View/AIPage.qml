import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front

Item {
    id: root

    // Page Name Property
    property string pageName: "AI"
    signal requestPage(string pageName)

    Rectangle {
        anchors.fill: parent
        color: Theme.bgPrimary

        Text {
            anchors.centerIn: parent
            text: "AI 분석 가능한 온라인 카메라가 없습니다."
            color: "#666688"
            font.pixelSize: 16
            horizontalAlignment: Text.AlignHCenter
            // 실제 표시 항목이 없을 때만 보임 (아래 GridView로 판단)
            visible: root.hanwhaOnlineCount === 0
        }

        property int hanwhaOnlineCount: {
            let cnt = 0;
            if (typeof cameraModel !== "undefined" && cameraModel !== null) {
                for (let i = 0; i < cameraModel.rowCount(); i++) {
                    const idx = cameraModel.index(i, 0);
                    const type = cameraModel.data(idx, 263);  // CameraTypeRole -> Qt::UserRole + 7 (263)
                    const online = cameraModel.data(idx, 259);  // IsOnlineRole -> Qt::UserRole + 3 (259)
                    if (type === "HANWHA" && online)
                        cnt++;
                }
            }
            return cnt;
        }

        GridView {
            anchors.fill: parent
            anchors.margins: 20
            cellWidth: 340
            cellHeight: 260
            clip: true
            model: cameraModel

            delegate: Item {
                readonly property bool shouldShow: model.cameraType === "HANWHA" && model.isOnline

                width: GridView.view.cellWidth
                height: shouldShow ? GridView.view.cellHeight : 0
                visible: shouldShow

                CameraCard {
                    anchors.centerIn: parent
                    width: parent.width - 20
                    height: parent.height - 20
                    title: model.title
                    rtspUrl: model.rtspUrl
                    isOnline: model.isOnline
                }
            }
        }
    }
}
