cmake_minimum_required(VERSION 3.16)

qt_add_library(FrontEnd STATIC)

set_source_files_properties(
    Theme/Theme.qml
    Theme/Icon.qml
    Theme/Typography.qml
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
        Component/SystemButton.qml
        Component/SideNavPanel.qml
        Component/NavItem.qml
        Component/AlarmCard.qml
        Component/VideoSurface.qml
        Component/DeviceControlBar.qml
        Theme/Icon.qml
        Theme/Theme.qml
        Theme/Typography.qml
        View/HomePage.qml
        View/DashboardPage.qml
        View/AIPage.qml
        View/DevicePage.qml
        View/MyPage.qml
        View/LoginPage.qml
        View/SettingsPage.qml
        View/SignupPage.qml
    RESOURCES
        Assets/Core/Icons/Alarm.svg
        Assets/Core/Icons/Exit.svg
        Assets/Core/Icons/Help.svg
        Assets/Core/Icons/Maximize.svg
        Assets/Core/Icons/Minimize.svg
        Assets/Core/Icons/Option.svg
        Assets/Core/Logos/OnlyLogo.svg
        Assets/Core/Fonts/01HanwhaB.ttf
        Assets/Core/Fonts/02HanwhaR.ttf
        Assets/Core/Fonts/03HanwhaL.ttf
        Assets/Core/Fonts/04HanwhaGothicB.ttf
        Assets/Core/Fonts/05HanwhaGothicR.ttf
        Assets/Core/Fonts/06HanwhaGothicL.ttf
        Assets/Core/Fonts/07HanwhaGothicEL.ttf
        Assets/Core/Fonts/08HanwhaGothicT.ttf
)

target_link_libraries(FrontEnd PRIVATE
    Qt6::Quick
    Qt6::Svg
    Qt6::Qml
    Qt6::Core
    Qt6::QuickControls2
    QWindowKit::Quick
)
