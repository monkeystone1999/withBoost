cmake_minimum_required(VERSION 3.16)


add_subdirectory(Assets)

qt_add_library(FrontEnd STATIC)

set_source_files_properties(
    theme/Theme.qml
    theme/Icon.qml
    theme/Typography.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(FrontEnd
    URI AnoMap.front
    QML_FILES
        Main.qml
        Titlebar.qml
        Component/BasicButton.qml
        Component/CameraCard.qml
        Component/CameraWindow.qml
        Component/CameraWindowTitlebar.qml
        Component/InputText.qml
        Component/SampleComponent.qml
        Component/SystemButton.qml
        Theme/Icon.qml
        Theme/Theme.qml
        Theme/Typography.qml
        View/DashboardPage.qml
        View/LoginPage.qml
        View/SettingsPage.qml
        View/SignupPage.qml
)

target_link_libraries(FrontEnd PRIVATE
    Qt6::Quick
    Qt6::Svg
    Qt6::Qml
    Qt6::Core
    Qt6::QuickControls2
    QWindowKit::Quick
    HanwhaAssets
)
