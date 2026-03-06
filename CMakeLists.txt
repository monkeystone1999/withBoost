cmake_minimum_required(VERSION 3.16)

project(AnoMap VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS
    Quick Svg Core Qml QuickControls2 Network
)

# ===== FFmpeg =====
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(FFmpeg REQUIRED)
include(SelectLibraryConfigurations)

function(anomap_ensure_ffmpeg_target component)
    if(TARGET FFmpeg::${component})
        return()
    endif()

    set(_var "FFmpeg_${component}")
    find_library(${_var}_LIBRARY_RELEASE NAMES ${component})
    find_library(${_var}_LIBRARY_DEBUG NAMES ${component}d ${component})
    select_library_configurations(${_var})

    if(${_var}_LIBRARY)
        add_library(FFmpeg::${component} UNKNOWN IMPORTED)
        if(${_var}_LIBRARY_RELEASE)
            set_property(TARGET FFmpeg::${component} APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
            set_target_properties(FFmpeg::${component} PROPERTIES
                IMPORTED_LOCATION_RELEASE "${${_var}_LIBRARY_RELEASE}"
            )
        endif()
        if(${_var}_LIBRARY_DEBUG)
            set_property(TARGET FFmpeg::${component} APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
            set_target_properties(FFmpeg::${component} PROPERTIES
                IMPORTED_LOCATION_DEBUG "${${_var}_LIBRARY_DEBUG}"
            )
        endif()
        if(DEFINED FFMPEG_INCLUDE_DIRS)
            set_target_properties(FFmpeg::${component} PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_INCLUDE_DIRS}"
            )
        endif()
    endif()
endfunction()

anomap_ensure_ffmpeg_target(swscale)
anomap_ensure_ffmpeg_target(swresample)



# ===== OpenCV =====
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui videoio)

# ===== QWindowKit Configuration =====
include(SetupQWindowKit)

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
