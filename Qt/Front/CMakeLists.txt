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
    theme/Theme.qml
    theme/Icon.qml
    theme/Typography.qml
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
        components/BasicButton.qml
        components/camera/CameraCard.qml
        components/camera/CameraWindow.qml
        components/camera/CameraWindowTitleBar.qml
        components/dashboard/GridColumnSelector.qml
        components/dashboard/AdminStatusBar.qml
        components/dashboard/DashboardContextMenu.qml
        components/camera/AiContextMenu.qml
        components/camera/AiControlBar.qml
        components/camera/DeviceContextMenu.qml
        components/InputText.qml
        components/SystemButton.qml
        components/SideNavPanel.qml
        components/NavItem.qml
        components/AlarmCard.qml
        components/VideoSurface.qml
        components/device/DeviceControlBar.qml
        components/device/DeviceBadge.qml
        components/device/DeviceControlPanel.qml
        components/device/LiveGraph.qml
        components/user/UserCard.qml
        theme/Icon.qml
        theme/Theme.qml
        theme/Typography.qml
        layouts/DashboardLayout.qml
        layouts/DeviceListLayout.qml
        layouts/UserListLayout.qml
        layouts/CameraSplitLayout.qml
        pages/DashboardPage.qml
        pages/AiPage.qml
        pages/DevicePage.qml
        pages/MyPage.qml
        pages/LoginPage.qml
        pages/SettingsPage.qml
        pages/SignupPage.qml
        pages/AdminPage.qml

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
# 1. 신규 페이지 추가: pages/ 폴더에 .qml 생성 후 모듈(QML_FILES)에 등록
# 2. 아이콘/에셋 추가: Assets/ 폴더 내 적절한 위치에 복사 후 RESOURCES에 경로 추가
# 3. 커스텀 컴포넌트: components/ 내에서 재사용 가능한 단위로 분리하여 관리
