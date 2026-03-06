# ===== QWindowKit Configuration =====
# qmsetup spawns an external CMake process, so keep Qt6_DIR in cache for sub-builds.
if(NOT Qt6_DIR)
    if(DEFINED ENV{Qt6_DIR} AND EXISTS "$ENV{Qt6_DIR}")
        set(Qt6_DIR "$ENV{Qt6_DIR}")
    elseif(DEFINED ENV{Qt6_ROOT} AND EXISTS "$ENV{Qt6_ROOT}/lib/cmake/Qt6")
        set(Qt6_DIR "$ENV{Qt6_ROOT}/lib/cmake/Qt6")
    endif()
endif()

if(NOT Qt6_DIR)
    message(FATAL_ERROR "Qt6_DIR is not set. Set Qt6_DIR (path to Qt6Config.cmake directory) or Qt6_ROOT.")
endif()

set(Qt6_DIR "${Qt6_DIR}" CACHE PATH "Path to Qt6Config.cmake directory" FORCE)

include(FetchContent)
FetchContent_Declare(
    QWindowKit
    GIT_REPOSITORY https://github.com/stdware/qwindowkit.git
    GIT_TAG        1.5.0
)

set(QWINDOWKIT_BUILD_QUICK ON CACHE BOOL "Build QWindowKit Quick module" FORCE)
FetchContent_MakeAvailable(QWindowKit)