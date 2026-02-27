import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.front
import AnoMap.back

Item {
    id: root

    // Navigation signal
    signal requestPage(string pageName)

    Login {
        id: loginController
        onLoginSuccess: {
            root.requestPage("Dashboard");
        }
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.bgPrimary

        ColumnLayout {
            anchors.centerIn: parent
            spacing: 25

            InputText {
                id: idField
                placeholder: "ID"
                rowLength: 200
                heightLength: 40
                onTextChanged: loginController.id = text
            }

            InputText {
                id: pwField
                placeholder: "Password"
                rowLength: 200
                heightLength: 40
                echoMode: TextInput.Password
                onTextChanged: loginController.password = text
                isError: loginController.isError
                errorText: loginController.errorMessage
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 20

                BasicButton {
                    text: "Login"
                    onClicked: {
                        console.log("Login button clicked! ID:", idField.text, "PW:", pwField.text);
                        loginController.login(idField.text, pwField.text);
                    }
                }

                BasicButton {
                    text: "Sign Up"
                    onClicked: root.requestPage("Signup")
                }
            }
        }
    }
}
