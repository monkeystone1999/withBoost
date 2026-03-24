include(FindPackageHandleStandardArgs)
include(SelectLibraryConfigurations)

# Diagnostic messages to trace loading
message(STATUS "Loading Custom FindFFmpeg.cmake from: ${CMAKE_CURRENT_LIST_FILE}")

# Define potential vcpkg search paths relative to project roots
set(_FFMPEG_SEARCH_PATHS
    "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg_installed/x64-windows"
    "${CMAKE_CURRENT_SOURCE_DIR}/build/vcpkg_installed/x64-windows"
    "${CMAKE_CURRENT_SOURCE_DIR}/../vcpkg_installed/x64-windows"
    "${PROJECT_SOURCE_DIR}/vcpkg_installed/x64-windows"
    "${PROJECT_SOURCE_DIR}/build/vcpkg_installed/x64-windows"
    "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows"
)
message(STATUS "FFmpeg Search Paths: ${_FFMPEG_SEARCH_PATHS}")

find_path(FFmpeg_INCLUDE_DIR
    NAMES libavcodec/avcodec.h
    PATH_SUFFIXES include
    PATHS ${_FFMPEG_SEARCH_PATHS}
)
message(STATUS "FFmpeg_INCLUDE_DIR: ${FFmpeg_INCLUDE_DIR}")

set(_FFmpeg_COMPONENTS avcodec avformat avutil avfilter swscale swresample)
set(FFmpeg_LIBRARIES "")

foreach(_comp ${_FFmpeg_COMPONENTS})
    find_library(FFmpeg_${_comp}_LIBRARY_RELEASE 
        NAMES ${_comp}
        PATH_SUFFIXES lib
        PATHS ${_FFMPEG_SEARCH_PATHS}
    )
    find_library(FFmpeg_${_comp}_LIBRARY_DEBUG 
        NAMES ${_comp}d ${_comp}
        PATH_SUFFIXES debug/lib
        PATHS ${_FFMPEG_SEARCH_PATHS}
    )
    
    select_library_configurations(FFmpeg_${_comp})
    message(STATUS "  Component ${_comp}: ${FFmpeg_${_comp}_LIBRARY}")
    
    if(FFmpeg_${_comp}_LIBRARY)
        list(APPEND FFmpeg_LIBRARIES ${FFmpeg_${_comp}_LIBRARY})
        
        if(NOT TARGET FFmpeg::${_comp})
            add_library(FFmpeg::${_comp} UNKNOWN IMPORTED)
            if(FFmpeg_${_comp}_LIBRARY_RELEASE)
                set_property(TARGET FFmpeg::${_comp} APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
                set_target_properties(FFmpeg::${_comp} PROPERTIES
                    IMPORTED_LOCATION_RELEASE "${FFmpeg_${_comp}_LIBRARY_RELEASE}"
                )
            endif()
            if(FFmpeg_${_comp}_LIBRARY_DEBUG)
                set_property(TARGET FFmpeg::${_comp} APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
                set_target_properties(FFmpeg::${_comp} PROPERTIES
                    IMPORTED_LOCATION_DEBUG "${FFmpeg_${_comp}_LIBRARY_DEBUG}"
                )
            endif()
            
            # Fallback for Generic IMPORTED_LOCATION
            if(FFmpeg_${_comp}_LIBRARY_RELEASE)
                set_target_properties(FFmpeg::${_comp} PROPERTIES IMPORTED_LOCATION "${FFmpeg_${_comp}_LIBRARY_RELEASE}")
            elseif(FFmpeg_${_comp}_LIBRARY_DEBUG)
                set_target_properties(FFmpeg::${_comp} PROPERTIES IMPORTED_LOCATION "${FFmpeg_${_comp}_LIBRARY_DEBUG}")
            endif()

            set_target_properties(FFmpeg::${_comp} PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_INCLUDE_DIR}"
            )
        endif()
    endif()
endforeach()

find_package_handle_standard_args(FFmpeg
    REQUIRED_VARS FFmpeg_LIBRARIES FFmpeg_INCLUDE_DIR
    HANDLE_COMPONENTS
)

if(FFmpeg_FOUND AND NOT TARGET FFmpeg::FFmpeg)
    add_library(FFmpeg::FFmpeg INTERFACE IMPORTED)
    set_target_properties(FFmpeg::FFmpeg PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_INCLUDE_DIR}"
    )
    foreach(_comp ${_FFmpeg_COMPONENTS})
        if(TARGET FFmpeg::${_comp})
            set_property(TARGET FFmpeg::FFmpeg APPEND PROPERTY INTERFACE_LINK_LIBRARIES FFmpeg::${_comp})
        endif()
    endforeach()
endif()
