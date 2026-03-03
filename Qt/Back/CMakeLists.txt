
qt_add_library(BackEnd STATIC
    Dashboard.cpp
    Login.cpp
    Signup.cpp
    Dashboard.hpp
    Login.hpp
    Signup.hpp
    CameraModel.hpp
    CameraModel.cpp
)

target_link_libraries(BackEnd PUBLIC
    Qt6::Core
)