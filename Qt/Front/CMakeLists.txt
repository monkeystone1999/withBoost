# @file Qt/Front/CMakeLists.txt
# @brief QML 프론트엔드 모듈 (FrontEnd) 구성 파일
#
# 이 파일은 프로젝트의 모든 QML 파일, 이미지 에셋(SVG), 폰트 및 테마 설정을 관리합니다.
# `AnoMap.Front`라는 URI를 통해 백엔드에서 등록된 C++ 객체들과 연결되어 사용자 인터페이스를 형성합니다.

cmake_minimum_required(VERSION 3.16)

# @brief FrontEnd 라이브러리 및 QML 싱글톤 설정
# 
# **표준 사용 방법론 (Standard Usage Methodology):**
# 1. **싱글톤 등록**: Theme, Icon 등 전역 테마 파일은 싱글톤 속성을 부여하여 어디서든 접근 가능케 함
# 2. **모듈 구성**: `qt_add_qml_module`을 통해 QML 파일과 리소스를 하나의 논리적 단위로 묶음
# 3. **QML 참조**: 다른 QML 파일에서 `import AnoMap.Front`를 통해 컴포넌트 사용
qt_add_library(FrontEnd STATIC)

set_source_files_properties(
    Theme/Theme.qml
    Theme/Icon.qml
    Theme/Typography.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
)

# @note QML 모듈 정의 (URI: AnoMap.Front)
#       모든 페이지, 컴포넌트, 레이아웃 및 에셋(Assets)을 포함합니다.
qt_add_qml_module(FrontEnd
    URI AnoMap.Front
    QML_FILES
        Main.qml
        TitleBar.qml
        Component/BasicButton.qml
        Component/Camera/CameraCard.qml
        Component/Camera/CameraWindow.qml
        Component/Camera/CameraWindowTitleBar.qml
        Component/Dashboard/GridColumnSelector.qml
        Component/Dashboard/AdminStatusBar.qml
        Component/Dashboard/DashboardContextMenu.qml
        Component/Camera/AiContextMenu.qml
        Component/Camera/AiControlBar.qml
        Component/Camera/DeviceContextMenu.qml
        Component/InputText.qml
        Component/SystemButton.qml
        Component/SideNavPanel.qml
        Component/NavItem.qml
        Component/AlarmCard.qml
        Component/VideoSurface.qml
        Component/Device/DeviceControlBar.qml
        Component/Device/DeviceBadge.qml
        Component/Device/DeviceControlPanel.qml
        Component/Device/LiveGraph.qml
        Component/User/UserCard.qml
        Theme/Icon.qml
        Theme/Theme.qml
        Theme/Typography.qml
        Layout/DashboardLayout.qml
        Layout/DeviceListLayout.qml
        Layout/UserListLayout.qml
        Layout/CameraSplitLayout.qml
        Page/DashboardPage.qml
        Page/AiPage.qml
        Page/DevicePage.qml
        Page/MyPage.qml
        Page/LoginPage.qml
        Page/SettingsPage.qml
        Page/SignupPage.qml
        Page/AdminPage.qml

    RESOURCES
        Assets/Core/Icons/Home.svg
        Assets/Core/Icons/Ai.svg
        Assets/Core/Icons/Device.svg
        Assets/Core/Icons/Profile.svg
        Assets/Core/Icons/Admin.svg
        Assets/Core/Icons/Logout.svg
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

# @note Qt6 그래픽 및 QML 관련 라이브러리 연결
target_link_libraries(FrontEnd PRIVATE
    Qt6::Quick
    Qt6::Svg
    Qt6::Qml
    Qt6::Core
    Qt6::QuickControls2
    QWindowKit::Quick
)

# [Frontend Workflow Guide]
# 1. 신규 페이지 추가: Page/ 폴더에 .qml 생성 후 모듈(QML_FILES)에 등록
# 2. 아이콘/에셋 추가: Assets/ 폴더 내 적절한 위치에 복사 후 RESOURCES에 경로 추가
# 3. 커스텀 컴포넌트: Component/ 내에서 재사용 가능한 단위로 분리하여 관리
