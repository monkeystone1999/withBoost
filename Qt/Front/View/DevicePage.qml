import QtQuick
import QtQuick.Controls
import AnoMap.front

Item {
    id: root
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
            pageType: "Device"
            selectedUrl: root.selectedUrl
            selectedSlotId: root.selectedSlotId

            onCameraSelected: (url, slotId) => {
                root.selectedUrl = url;
                root.selectedSlotId = slotId;
                console.log("Device Page: Camera selected", url, "slot", slotId);
            }

            onActionRequested: url => {
                console.log("Device Page: Device control requested for", url);
            // Device 컨트롤은 이미 우측 패널에 표시됨
            }

            onCameraRightClicked: (url, gx, gy) => {
                deviceCtxMenu.targetUrl = url;
                let mx = gx, my = gy;
                if (mx + deviceCtxMenu.width > root.width)
                    mx = root.width - deviceCtxMenu.width - 4;
                if (my + deviceCtxMenu.height > root.height)
                    my = root.height - deviceCtxMenu.height - 4;
                deviceCtxMenu.x = mx;
                deviceCtxMenu.y = my;
                deviceCtxMenu.visible = true;
            }
        }
    }

    // Context Menu
    MouseArea {
        anchors.fill: parent
        z: 399
        visible: deviceCtxMenu.visible
        onClicked: deviceCtxMenu.visible = false
    }

    DeviceContextMenu {
        id: deviceCtxMenu
        visible: false
        z: 400
        onDeviceControlRequested: url => {
            console.log("Device control from context menu for", url);
        }
        onAiControlRequested: url => {
            console.log("AI control from context menu for", url);
        // root.requestPage("AI") // if navigation is desired instead
        }
    }
}
