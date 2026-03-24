import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.Front

Item {
    id: rootItem
    signal requestPage(string pageName)
    signal requestClose

    Connections {
        target: signupController
        function onSignupSuccess(message) {
            console.log("Signup Success:", message);
            // Replace with requestPage directly to login if "Back" acts like "Login"
            // Usually "Back" returns to previous Login page
            rootItem.requestPage("Back");
        }
    }

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
                Layout.alignment: Qt.AlignHCenter
            }

            InputText {
                id: emailField
                placeholder: "Email Address"
                rowLength: 400
                heightLength: 40
                Layout.alignment: Qt.AlignHCenter
            }

            InputText {
                id: pwField
                placeholder: "Password"
                rowLength: 400
                heightLength: 40
                echoMode: TextInput.Password
                Layout.alignment: Qt.AlignHCenter
            }

            InputText {
                id: pwConfirmField
                placeholder: "Confirm Password"
                rowLength: 400
                heightLength: 40
                echoMode: TextInput.Password
                isError: signupController.isError
                errorText: signupController.errorMessage
                Layout.alignment: Qt.AlignHCenter
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.fillWidth: true
                Layout.topMargin: 20 // Keep at 45px total (25 spacing + 20 margin)

                Item {
                    Layout.fillWidth: true
                }

                Item {
                    Layout.preferredWidth: backBtn.width * 1.65
                    Layout.preferredHeight: backBtn.height * 1.65
                    BasicButton {
                        id: backBtn
                        anchors.centerIn: parent
                        text: "Back"
                        scale: 1.65
                        onClicked: rootItem.requestPage("Back")
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                Item {
                    Layout.preferredWidth: signupBtn.width * 1.65
                    Layout.preferredHeight: signupBtn.height * 1.65
                    BasicButton {
                        id: signupBtn
                        anchors.centerIn: parent
                        text: "Sign Up"
                        scale: 1.65
                        onClicked: {
                            console.log("Signup Process Triggered");
                            signupController.signup(nameField.text, emailField.text, pwField.text);
                        }
                    }
                }

                Item {
                    Layout.fillWidth: true
                }
            }
        }
    }
}
