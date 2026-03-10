import QtQuick
import QtQuick.Controls
import AnoMap.front
import "../Layout"
import "../Component/device"

Item {
    id: root
    signal requestPage(string pageName)
    signal requestClose

    Rectangle {
        anchors.fill: parent
        color: Theme.bgPrimary

        Text {
            visible: typeof deviceModel !== "undefined" && deviceModel.count === 0
            anchors.centerIn: parent
            text: "No controllable devices.\nWaiting for DEVICE list from server."
            color: "#666688"
            font.pixelSize: 16
            horizontalAlignment: Text.AlignHCenter
        }

        DeviceListLayout {
            anchors.fill: parent
            visible: typeof deviceModel !== "undefined" && deviceModel.count > 0
            model: typeof deviceModel !== "undefined" ? deviceModel : null

            onDeviceControlRequested: (ip, motor, ir, heater) => {
                if (typeof networkBridge !== "undefined" && networkBridge !== null) {
                    networkBridge.sendDevice(ip, motor, ir, heater);
                }
            }
        }
    }
}
