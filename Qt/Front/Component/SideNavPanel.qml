import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import Qt5Compat.GraphicalEffects
import AnoMap.Front

Item {
    id: rootItem

    property string currentPage: "Dashboard"

    signal requestPage(string pageName)

    property bool isOpen: false

    function open() {
        isOpen = true;
    }
    function close() {
        isOpen = false;
    }
    function toggle() {
        isOpen = !isOpen;
    }

    anchors.fill: parent
    enabled: isOpen

    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: rootItem.isOpen ? 0.45 : 0.0
        visible: opacity > 0
        Behavior on opacity {
            NumberAnimation {
                duration: 220
                easing.type: Easing.OutCubic
            }
        }
        MouseArea {
            anchors.fill: parent
            onClicked: rootItem.close()
        }
    }

    Rectangle {
        id: panel
        width: 260
        height: parent.height
        x: rootItem.isOpen ? 0 : -width
        color: Theme.bgSecondary

        Behavior on x {
            NumberAnimation {
                duration: 250
                easing.type: Easing.OutCubic
            }
        }

        Rectangle {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: 1
            color: Theme.isDark ? "#3A3A3C" : "#D1D1D6"
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.topMargin: 16
            anchors.bottomMargin: 16
            spacing: 0

            Row {
                Layout.leftMargin: 20
                Layout.bottomMargin: 8
                spacing: 10

                Image {
                    readonly property int logoH: 30
                    readonly property int logoW: logoH * (85 / 77)
                    anchors.verticalCenter: parent.verticalCenter
                    source: Icon.logo
                    height: logoH
                    width: logoW
                    sourceSize.width: logoW
                    sourceSize.height: logoH
                    fillMode: Image.PreserveAspectFit
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: "AnoMAP"
                    font.family: Typography.fontFamilyPrimary
                    font.pixelSize: 22
                    color: Theme.fontColor
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                Layout.bottomMargin: 8
                height: 1
                color: Theme.isDark ? "#3A3A3C" : "#D1D1D6"
            }

            Repeater {
                model: [
                    {
                        label: "Home",
                        icon: Icon.homeIcon,
                        page: "Dashboard"
                    },
                    {
                        label: "AI",
                        icon: Icon.aiIcon,
                        page: "AI"
                    },
                    {
                        label: "Device",
                        icon: Icon.deviceIcon,
                        page: "Device"
                    },
                    {
                        label: "My Page",
                        icon: Icon.profileIcon,
                        page: "MyPage"
                    },
                    {
                        label: "Admin",
                        icon: Icon.adminIcon,
                        page: "AdminDashboard"
                    }
                ]
                delegate: NavItem {
                    Layout.fillWidth: true
                    label: modelData.label
                    iconSource: modelData.icon
                    isActive: rootItem.currentPage === modelData.page
                    visible: modelData.page !== "AdminDashboard" || (typeof loginController !== "undefined" && loginController.state === "admin")
                    onClicked: {
                        rootItem.currentPage = modelData.page;
                        rootItem.requestPage(modelData.page);
                        rootItem.close();
                    }
                }
            }

            Item {
                Layout.fillHeight: true
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.leftMargin: 16
                Layout.rightMargin: 16
                Layout.topMargin: 8
                Layout.bottomMargin: 8
                height: 1
                color: Theme.isDark ? "#3A3A3C" : "#D1D1D6"
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 56
                spacing: 0

                // Logout Button
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 56
                    color: logoutHoverArea.containsMouse ? (Theme.isDark ? "#3A3A3C" : "#E5E5EA") : "transparent"

                    Row {
                        anchors.centerIn: parent
                        spacing: 8
                        Item {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 20
                            height: 20
                            Image {
                                id: logoutIconImg
                                anchors.fill: parent
                                source: Icon.logoutIcon
                                sourceSize.width: 20
                                sourceSize.height: 20
                                fillMode: Image.PreserveAspectFit
                                visible: false
                            }
                            ColorOverlay {
                                anchors.fill: logoutIconImg
                                source: logoutIconImg
                                color: Theme.fontColor
                            }
                        }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Logout"
                            font.family: Typography.fontFamilySecondary
                            font.pixelSize: Typography.thirdFont
                            color: Theme.fontColor
                        }
                    }

                    MouseArea {
                        id: logoutHoverArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (typeof appController !== "undefined")
                                appController.logout();
                            rootItem.currentPage = "Login";
                            rootItem.close();
                        }
                    }
                }

                Rectangle {
                    Layout.preferredHeight: 24
                    Layout.alignment: Qt.AlignVCenter
                    width: 1
                    color: Theme.isDark ? "#3A3A3C" : "#D1D1D6"
                }

                // Option Button
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 56
                    color: optionHoverArea.containsMouse ? (Theme.isDark ? "#3A3A3C" : "#E5E5EA") : "transparent"

                    Row {
                        anchors.centerIn: parent
                        spacing: 8

                        Item {
                            anchors.verticalCenter: parent.verticalCenter
                            width: 20
                            height: 20
                            Image {
                                id: optIcon
                                anchors.fill: parent
                                source: Icon.option
                                sourceSize.width: 20
                                sourceSize.height: 20
                                visible: false
                            }
                            ColorOverlay {
                                anchors.fill: optIcon
                                source: optIcon
                                color: Theme.fontColor
                            }
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Option"
                            font.family: Typography.fontFamilySecondary
                            font.pixelSize: Typography.thirdFont
                            color: Theme.fontColor
                        }
                    }

                    MouseArea {
                        id: optionHoverArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (typeof appController !== "undefined")
                                appController.openOptionDialog();
                            rootItem.close();
                        }
                    }
                }
            }
        }
    }
}
