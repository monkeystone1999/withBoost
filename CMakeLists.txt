# @file CMakeLists.txt
# @brief AnoMap 프로젝트의 루트 빌드 구성 파일
#
# 이 파일은 프로젝트의 전역 설정, 의존성 관리(Qt, FFmpeg, QWindowKit), 
# 그리고 하위 디렉토리(Qt/Front, Qt/Back, Src)의 빌드 순서를 제어합니다.
# 
# **표준 빌드 방법론:**
# 1. 외부 의존성(Qt 6.8+, FFmpeg) 설치 확인
# 2. `cmake -B build` 명령으로 빌드 시스템 생성
# 3. `cmake --build build` 명령으로 전체 프로젝트 컴파일

cmake_minimum_required(VERSION 3.16)

project(AnoMap VERSION 0.5 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# @note Qt6 프레임워크 로드 (필수 컴포넌트: Quick, Core, Network 등)
find_package(Qt6 REQUIRED COMPONENTS
    Quick Svg Core Qml QuickControls2 Network ShaderTools Multimedia
)


# ===== FFmpeg =====
# @note FFmpeg 모듈 로드. 
#       vcpkg 연동을 위해 커스텀 모듈을 직접 include 합니다.
include(cmake/FindFFmpeg.cmake)


# ===== QWindowKit Configuration =====
# @note 커스텀 윈도우 프레임을 위한 QWindowKit 라이브러리 설정
include(SetupQWindowKit)

qt_standard_project_setup(REQUIRES 6.8)

# @brief 메인 실행 파일 생성 및 핵심 소스 등록
qt_add_executable(AnoMap
    App.cpp
    Core.hpp
    Core.cpp
)

# @note 각 레이어별 하위 디렉토리 추가 (빌드 순서에 주의)
add_subdirectory(Qt/Front)
add_subdirectory(Qt/Back)
add_subdirectory(Src)

set_target_properties(AnoMap PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# @note 실행 파일과 라이브러리/프레임워크 링크 설정
target_link_libraries(AnoMap PRIVATE
    Qt6::Quick
    Qt6::Svg
    Qt6::Qml
    Qt6::Core
    Qt6::QuickControls2
    Qt6::Multimedia
    QWindowKit::Quick
    BackEnd
    AnoMapSTD
    FrontEnd
    FrontEndplugin
    FFmpeg::avcodec
    FFmpeg::avformat
    FFmpeg::avutil
    FFmpeg::swscale
    FFmpeg::swresample
)

qt_import_qml_plugins(AnoMap)

include(GNUInstallDirs)
install(TARGETS AnoMap
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# [Build Workflow Guide]
# 1. 구성(Configure): 프로젝트 루트에서 cmake -B build 실행
# 2. 빌드(Build): cmake --build build 실행
# 3. 설치(Install): 빌드 성공 후 결과물이 bin 디렉토리에 생성됨을 확인
