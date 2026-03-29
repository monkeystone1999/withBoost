import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Qt5Compat.GraphicalEffects
import AnoMap.Front

Item {
    id: rootItem
    signal requestClose

    // Background blur applying over the previous view
    Rectangle {
        id: bgDimmer
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.4)
    }

    // Instead of blurring the whole window recursively (which is slow/complex in pure QML without an underlying ShaderEffectSource of the window),
    // we rely on the dimming overlay to provide the "focus" effect requested, but we can enable FastBlur if a source is provided.
    // For now, robust semi-transparent black overlay performs a reliable focus-pull.
    MouseArea {
        anchors.fill: parent
        onClicked: rootItem.requestClose()
    }

    Rectangle {
        width: 400
        height: 500
        anchors.centerIn: parent
        color: Theme.bgSecondary
        radius: 12

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 30

            Text {
                text: "Settings"
                color: Theme.fontColor
                font.pixelSize: 28
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 10
            }

            RowLayout {
                spacing: 20
                Layout.alignment: Qt.AlignHCenter
                Text {
                    text: "Dark Mode"
                    color: Theme.fontColor
                    font.pixelSize: 18
                }
                Switch {
                    checked: Theme.isDark
                    onToggled: Theme.isDark = !Theme.isDark
                }
            }

            RowLayout {
                spacing: 20
                Layout.alignment: Qt.AlignHCenter

                Text {
                    text: "Default Resolution"
                    color: Theme.fontColor
                    font.pixelSize: 18
                }
                ComboBox {
                    id: resolutionCombo
                    model: typeof settingsController !== "undefined" ? settingsController.resolutions : ["1920x1080"]
                    currentIndex: typeof settingsController !== "undefined" ? settingsController.resolutions.indexOf(settingsController.defaultResolution) : 0
                    onActivated: {
                        if (typeof settingsController !== "undefined") {
                            settingsController.defaultResolution = currentText;
                            settingsController.save();
                        }
                    }
                }
            }

            Item {
                Layout.preferredHeight: 30
                Layout.preferredWidth: 1
            }

            BasicButton {
                text: "Close"
                Layout.alignment: Qt.AlignHCenter
                onClicked: rootItem.requestClose()
            }
        }
    }
}
