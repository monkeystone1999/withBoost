import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front

Item {
    id: root
    signal requestPage(string pageName)

    Rectangle {
        anchors.fill: parent
        color: Theme.bgPrimary

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 20

            Text {
                text: "Create Account"
                color: Theme.fontColor
                font.pixelSize: 32
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
                Layout.bottomMargin: 20
            }

            InputText {
                id: nameField
                placeholder: "Full Name"
                rowLength: 400
                heightLength: 40
            }

            InputText {
                id: emailField
                placeholder: "Email Address"
                rowLength: 400
                heightLength: 40
            }

            InputText {
                id: pwField
                placeholder: "Password"
                rowLength: 400
                heightLength: 40
                echoMode: TextInput.Password
            }

            InputText {
                id: pwConfirmField
                placeholder: "Confirm Password"
                rowLength: 400
                heightLength: 40
                echoMode: TextInput.Password
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 20
                Layout.topMargin: 20

                BasicButton {
                    text: "Back"
                    onClicked: root.requestPage("Back")
                }

                BasicButton {
                    text: "Sign Up"
                    onClicked: {
                        console.log("Signup Process Triggered");
                        // 가입 로직은 나중에 추가. 일단 완료됐다고 가정하고 돌아감.
                        root.requestPage("Back");
                    }
                }
            }
        }
    }
}
