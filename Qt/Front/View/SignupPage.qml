import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front

Item {
    id: root
    signal requestPage(string pageName)
    signal requestClose

    Connections {
        target: signupController
        function onSignupSuccess(message) {
            console.log("Signup Success:", message);
            // Replace with requestPage directly to login if "Back" acts like "Login"
            // Usually "Back" returns to previous Login page
            root.requestPage("Back");
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
                isError: signupController.isError
                errorText: signupController.errorMessage
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
                        signupController.signup(nameField.text, emailField.text, pwField.text);
                    }
                }
            }
        }
    }
}
