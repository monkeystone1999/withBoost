cmake_minimum_required(VERSION 3.16)

project(AnoMap VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS
    Quick Svg Core Qml QuickControls2 Network
)

# ===== FFmpeg =====
# Bypass FindFFMPEG.cmake entirely — use find_library directly into the
# vcpkg-managed installed directory and create proper imported targets.
set(_FFmpeg_ROOT "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows")

find_path(FFmpeg_INCLUDE_DIR
    NAMES libavcodec/avcodec.h
    PATHS "${_FFmpeg_ROOT}/include"
    NO_DEFAULT_PATH
    REQUIRED
)

foreach(_comp avcodec avformat avutil avfilter swscale swresample)
    find_library(FFmpeg_${_comp}_LIB_RELEASE
        NAMES ${_comp}
        PATHS "${_FFmpeg_ROOT}/lib"
        NO_DEFAULT_PATH REQUIRED
    )
    find_library(FFmpeg_${_comp}_LIB_DEBUG
        NAMES ${_comp}
        PATHS "${_FFmpeg_ROOT}/debug/lib"
        NO_DEFAULT_PATH REQUIRED
    )
    add_library(FFmpeg::${_comp} IMPORTED STATIC GLOBAL)
    set_target_properties(FFmpeg::${_comp} PROPERTIES
        IMPORTED_LOCATION         "${FFmpeg_${_comp}_LIB_RELEASE}"
        IMPORTED_LOCATION_RELEASE "${FFmpeg_${_comp}_LIB_RELEASE}"
        IMPORTED_LOCATION_DEBUG   "${FFmpeg_${_comp}_LIB_DEBUG}"
        INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_INCLUDE_DIR}"
    )
endforeach()



# ===== OpenCV =====
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui videoio)

# ===== QWindowKit Configuration =====
# Pass Qt6 prefix into QWindowKit's sub-builds (qmsetup uses ExternalProject
# which spawns a separate cmake process that does not inherit CMAKE_PREFIX_PATH)
set(Qt6_DIR "${CMAKE_PREFIX_PATH}/lib/cmake/Qt6" CACHE STRING "" FORCE)

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
    FrontEndplugin
    FFmpeg::avcodec
    FFmpeg::avformat
    FFmpeg::avutil
    FFmpeg::avfilter
    FFmpeg::swscale
    FFmpeg::swresample
    ${OpenCV_LIBS}
)

# qt_import_qml_plugins 는 자동 생성이 필요해서 위에서 FrontEndplugin 직접 링크로 대체
qt_import_qml_plugins(AnoMap)

include(GNUInstallDirs)
install(TARGETS AnoMap
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
