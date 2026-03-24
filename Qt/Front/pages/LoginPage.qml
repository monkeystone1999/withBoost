import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import AnoMap.Front

Item {
    id: rootItem

    // Navigation signal
    signal requestPage(string pageName)
    signal requestClose

    Connections {
        target: loginController
        function onLoginSuccess() {
            if (loginController.state === "admin") {
                rootItem.requestPage("AdminDashboard");
            } else {
                rootItem.requestPage("Dashboard");
            }
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
                rowLength: 300
                heightLength: 40
                Layout.alignment: Qt.AlignHCenter
            }

            InputText {
                id: pwField
                placeholder: "Password"
                rowLength: 300
                heightLength: 40
                echoMode: TextInput.Password
                isError: loginController.isError
                errorText: loginController.errorMessage
                Layout.alignment: Qt.AlignHCenter
            }

            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                Layout.preferredWidth: 350
                Layout.topMargin: 17.5 // 25 (spacing) * 1.7 = 42.5 -> +17.5

                Item {
                    Layout.fillWidth: true
                }

                Item {
                    Layout.preferredWidth: loginBtn.width * 1.65
                    Layout.preferredHeight: loginBtn.height * 1.65
                    BasicButton {
                        id: loginBtn
                        anchors.centerIn: parent
                        text: "Login"
                        scale: 1.65
                        onClicked: {
                            console.log("Login button clicked! ID:", idField.text, "PW:", pwField.text);
                            loginController.login(idField.text, pwField.text);
                        }
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
                        onClicked: rootItem.requestPage("Signup")
                    }
                }

                Item {
                    Layout.fillWidth: true
                }
            }
        }
    }
}
