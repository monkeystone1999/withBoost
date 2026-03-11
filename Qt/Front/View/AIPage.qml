import QtQuick
import QtQuick.Controls
import AnoMap.front
import "../Layout"
import "../Component/camera"

Item {
    id: root

    property string pageName: "AI"
    signal requestPage(string pageName)
    signal requestClose

    property string selectedUrl: ""
    property int selectedSlotId: -1

    Rectangle {
        anchors.fill: parent
        color: Theme.bgPrimary

        CameraSplitLayout {
            anchors.fill: parent
            model: typeof cameraModel !== "undefined" ? cameraModel : null
            pageType: "AI"
            selectedUrl: root.selectedUrl
            selectedSlotId: root.selectedSlotId

            onCameraSelected: (url, slotId) => {
                root.selectedUrl = url;
                root.selectedSlotId = slotId;
                console.log("AI Page: Camera selected", url, "slot", slotId);
            }

            onActionRequested: url => {
                console.log("AI Page: AI action requested for", url);
            // TODO: AI 설정 다이얼로그 열기
            }

            onCameraRightClicked: (url, gx, gy) => {
                aiCtxMenu.targetUrl = url;
                let mx = gx, my = gy;
                if (mx + aiCtxMenu.width > root.width)
                    mx = root.width - aiCtxMenu.width - 4;
                if (my + aiCtxMenu.height > root.height)
                    my = root.height - aiCtxMenu.height - 4;
                aiCtxMenu.x = mx;
                aiCtxMenu.y = my;
                aiCtxMenu.visible = true;
            }
        }
    }

    // Context Menu (필요시)
    MouseArea {
        anchors.fill: parent
        z: 399
        visible: aiCtxMenu.visible
        onClicked: aiCtxMenu.visible = false
    }

    AiContextMenu {
        id: aiCtxMenu
        visible: false
        z: 400
        onAiControlRequested: url => {
            console.log("AI control requested for", url);
        }
        onViewInDeviceTabRequested: url => root.requestPage("Device")
    }
}
