cmake_minimum_required(VERSION 3.16)

project(AnoMap VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS
    Quick Svg Core Qml QuickControls2 Network
)

# ===== QWindowKit Configuration =====
include(FetchContent)
FetchContent_Declare(
    QWindowKit
    GIT_REPOSITORY https://github.com/stdware/qwindowkit.git
    GIT_TAG        1.5.0
)

set(QWINDOWKIT_BUILD_QUICK ON CACHE BOOL "Build QWindowKit Quick module" FORCE)
FetchContent_MakeAvailable(QWindowKit)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(AnoMap
    App.cpp
)

add_subdirectory(Qt/Front)
add_subdirectory(Qt/Back)
add_subdirectory(Src)

set_target_properties(AnoMap PROPERTIES
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(AnoMap PRIVATE
    Qt6::Quick
    Qt6::Svg
    Qt6::Qml
    Qt6::Core
    Qt6::QuickControls2
    QWindowKit::Quick
    BackEnd
    AnoMapSTD
    FrontEnd
)

include(GNUInstallDirs)
install(TARGETS AnoMap
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
