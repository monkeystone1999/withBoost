
qt_add_library(BackEnd STATIC
    Login.cpp
    Login.hpp
    Signup.cpp
    Signup.hpp
    CameraModel.hpp
    CameraModel.cpp
    DeviceModel.hpp
    DeviceModel.cpp
    NetworkBridge.hpp
    NetworkBridge.cpp
    AlarmManager.hpp
    AlarmManager.cpp
    VideoWorker.hpp
    VideoWorker.cpp
    VideoManager.hpp
    VideoManager.cpp
    VideoSurfaceItem.hpp
    VideoSurfaceItem.cpp
    ServerStatusModel.hpp
    ServerStatusModel.cpp
)

target_link_libraries(BackEnd PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    AnoMapSTD
)

target_include_directories(BackEnd PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src
)
