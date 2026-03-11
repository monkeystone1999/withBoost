
qt_add_library(BackEnd STATIC
    Auth.cpp
    Auth.hpp
    Camera.hpp
    Camera.cpp
    Device.hpp
    Device.cpp
    NetworkBridge.hpp
    NetworkBridge.cpp
    AlarmManager.hpp
    AlarmManager.cpp
    VideoStream.hpp
    VideoStream.cpp
    VideoFrameTexture.hpp
    VideoFrameTexture.cpp
    VideoSurfaceItem.hpp
    VideoSurfaceItem.cpp
    ServerStatus.hpp
    ServerStatus.cpp
    UserModel.cpp
    UserModel.hpp
)

target_link_libraries(BackEnd PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::GuiPrivate
    AnoMapSTD
)

target_include_directories(BackEnd PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src
)
