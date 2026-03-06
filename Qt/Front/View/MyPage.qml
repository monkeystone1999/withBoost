import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front

Item {
    id: root

    // Page Name Property
    property string pageName: "MyPage"
    signal requestPage(string pageName)

    Rectangle {
        anchors.fill: parent
        color: Theme.bgPrimary

        ScrollView {
            anchors.fill: parent
            contentWidth: parent.width
            clip: true

            ColumnLayout {
                width: parent.width
                spacing: 20

                // Top Margin
                Item {
                    Layout.preferredHeight: 30
                }

                // 로고 또는 아이콘 영역 (옵션)
                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 80
                    height: 80
                    radius: 40
                    color: (typeof Theme !== 'undefined' && Theme.colorSecondary) ? Theme.colorSecondary : "#88aaaa"

                    Text {
                        anchors.centerIn: parent
                        text: (typeof loginController !== 'undefined' && loginController !== null && loginController.username && loginController.username.length > 0) ? loginController.username.substring(0, 1).toUpperCase() : "?"
                        color: (typeof Theme !== 'undefined' && Theme.colorBackground) ? Theme.colorBackground : "#ffffff"
                        font.family: (typeof Typography !== 'undefined' && Typography.fontFamilyPrimary) ? Typography.fontFamilyPrimary : "sans-serif"
                        font.pixelSize: 36
                        font.weight: Font.Bold
                    }
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("환영합니다, %1 님").arg((typeof loginController !== 'undefined' && loginController !== null && loginController.username) ? loginController.username : "알 수 없음")
                    color: (typeof Theme !== 'undefined' && Theme.fontColor) ? Theme.fontColor : "#ffffff"
                    font.family: (typeof Typography !== 'undefined' && Typography.fontFamilyPrimary) ? Typography.fontFamilyPrimary : "sans-serif"
                    font.pixelSize: 28
                    font.weight: Font.Bold
                }

                RowLayout {
                    Layout.alignment: Qt.AlignHCenter
                    spacing: 15

                    Text {
                        text: qsTr("권한 그룹:")
                        color: (typeof Theme !== 'undefined' && Theme.fontColorSecondary) ? Theme.fontColorSecondary : "#aaaaaa"
                        font.family: (typeof Typography !== 'undefined' && Typography.fontFamilyPrimary) ? Typography.fontFamilyPrimary : "sans-serif"
                        font.pixelSize: 18
                    }

                    Rectangle {
                        color: (typeof loginController !== 'undefined' && loginController !== null && loginController.state === "admin") ? ((typeof Theme !== 'undefined' && Theme.colorPrimary) ? Theme.colorPrimary : "#ff5555") : ((typeof Theme !== 'undefined' && Theme.colorSecondary) ? Theme.colorSecondary : "#88aaaa")
                        radius: 4
                        width: groupText.implicitWidth + 16
                        Layout.preferredHeight: 30

                        Text {
                            id: groupText
                            anchors.centerIn: parent
                            text: (typeof loginController !== 'undefined' && loginController !== null && loginController.state) ? loginController.state.toUpperCase() : "USER"
                            color: (typeof Theme !== 'undefined' && Theme.colorBackground) ? Theme.colorBackground : "#ffffff"
                            font.family: (typeof Typography !== 'undefined' && Typography.fontFamilyPrimary) ? Typography.fontFamilyPrimary : "sans-serif"
                            font.pixelSize: 14
                            font.weight: Font.Bold
                        }
                    }
                }

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: parent.width * 0.8
                    height: 1
                    color: (typeof Theme !== 'undefined' && Theme.colorSecondary) ? Theme.colorSecondary : "#88aaaa"
                    opacity: 0.3
                    Layout.topMargin: 20
                    Layout.bottomMargin: 10
                }

                // ── Settings List ────────────────────────────────────────────────
                ColumnLayout {
                    Layout.alignment: Qt.AlignHCenter
                    width: Math.min(600, parent.width * 0.8)
                    spacing: 15

                    Text {
                        text: "Account Settings"
                        color: "#aaccff"
                        font.pixelSize: 20
                        font.bold: true
                        Layout.bottomMargin: 10
                    }

                    // 1. Username Change Setting
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: usernameCol.implicitHeight + 30
                        color: "#1a1a2e"
                        radius: 8
                        border.color: "#334466"
                        border.width: 1

                        ColumnLayout {
                            id: usernameCol
                            anchors.fill: parent
                            anchors.margins: 15
                            spacing: 10

                            RowLayout {
                                Layout.fillWidth: true
                                Text {
                                    text: "Change Username"
                                    color: "white"
                                    font.pixelSize: 16
                                    font.bold: true
                                    Layout.fillWidth: true
                                }
                            }
                            TextField {
                                id: newUsernameInput
                                Layout.fillWidth: true
                                Layout.preferredHeight: 40
                                placeholderText: "Enter new username"
                                color: "white"
                                font.pixelSize: 14
                                background: Rectangle {
                                    color: "#2a2a3e"
                                    radius: 4
                                    border.color: newUsernameInput.activeFocus ? "#6688ff" : "#445577"
                                    border.width: 1
                                }
                            }
                            Button {
                                text: "Update Username"
                                Layout.alignment: Qt.AlignRight
                                Layout.preferredHeight: 36
                                Layout.preferredWidth: 140
                                background: Rectangle {
                                    color: parent.down ? "#4466aa" : (parent.hovered ? "#5588cc" : "#3366aa")
                                    radius: 4
                                }
                                contentItem: Text {
                                    text: parent.text
                                    color: "white"
                                    font.pixelSize: 14
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                onClicked: {
                                    if (newUsernameInput.text.trim().length > 0) {
                                        // TODO: Add backend API call here
                                        console.log("Request username change to:", newUsernameInput.text);
                                        newUsernameInput.text = "";
                                    }
                                }
                            }
                        }
                    }

                    // 2. Password Change Setting
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: passwordCol.implicitHeight + 30
                        color: "#1a1a2e"
                        radius: 8
                        border.color: "#334466"
                        border.width: 1

                        ColumnLayout {
                            id: passwordCol
                            anchors.fill: parent
                            anchors.margins: 15
                            spacing: 10

                            RowLayout {
                                Layout.fillWidth: true
                                Text {
                                    text: "Change Password"
                                    color: "white"
                                    font.pixelSize: 16
                                    font.bold: true
                                    Layout.fillWidth: true
                                }
                            }
                            TextField {
                                id: currentPasswordInput
                                Layout.fillWidth: true
                                Layout.preferredHeight: 40
                                placeholderText: "Enter current password"
                                echoMode: TextInput.Password
                                color: "white"
                                font.pixelSize: 14
                                background: Rectangle {
                                    color: "#2a2a3e"
                                    radius: 4
                                    border.color: currentPasswordInput.activeFocus ? "#6688ff" : "#445577"
                                    border.width: 1
                                }
                            }
                            TextField {
                                id: newPasswordInput
                                Layout.fillWidth: true
                                Layout.preferredHeight: 40
                                placeholderText: "Enter new password"
                                echoMode: TextInput.Password
                                color: "white"
                                font.pixelSize: 14
                                background: Rectangle {
                                    color: "#2a2a3e"
                                    radius: 4
                                    border.color: newPasswordInput.activeFocus ? "#6688ff" : "#445577"
                                    border.width: 1
                                }
                            }
                            Button {
                                id: updatePasswordBtn
                                text: "Update Password"
                                Layout.alignment: Qt.AlignRight
                                Layout.preferredHeight: 36
                                Layout.preferredWidth: 140
                                background: Rectangle {
                                    color: parent.down ? "#4466aa" : (parent.hovered ? "#5588cc" : "#3366aa")
                                    radius: 4
                                }
                                contentItem: Text {
                                    text: parent.text
                                    color: "white"
                                    font.pixelSize: 14
                                    font.bold: true
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                }
                                onClicked: {
                                    if (currentPasswordInput.text.trim().length === 0 || newPasswordInput.text.trim().length === 0) {
                                        console.log("Passwords cannot be empty");
                                        return;
                                    }
                                    // TODO: Replace with real verification
                                    console.log("Request password change");
                                    currentPasswordInput.text = "";
                                    newPasswordInput.text = "";
                                }
                            }
                        }
                    }

                    // Bottom Margin
                    Item {
                        Layout.preferredHeight: 50
                    }
                }
            }
        }
    }
}
