include(FindPackageHandleStandardArgs)
include(SelectLibraryConfigurations)

find_path(FFmpeg_INCLUDE_DIR
    NAMES libavcodec/avcodec.h
)

set(_FFmpeg_COMPONENTS avcodec avformat avutil avfilter swscale swresample)
set(FFmpeg_LIBRARIES "")

foreach(_comp ${_FFmpeg_COMPONENTS})
    # vcpkg automatically handles find_library across Release/Debug layout
    find_library(FFmpeg_${_comp}_LIBRARY_RELEASE NAMES ${_comp})
    find_library(FFmpeg_${_comp}_LIBRARY_DEBUG NAMES ${_comp}d ${_comp})
    
    select_library_configurations(FFmpeg_${_comp})
    
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
            if(FFmpeg_${_comp}_LIBRARY)
                set_target_properties(FFmpeg::${_comp} PROPERTIES
                    IMPORTED_LOCATION "${FFmpeg_${_comp}_LIBRARY_RELEASE}"
                    INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_INCLUDE_DIR}"
                )
            endif()
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
