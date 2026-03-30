qt_add_library(BackEnd STATIC
    # Authentication
    Controllers/AuthController.hpp
    Controllers/AuthController.cpp
    # Models
    Models/CameraModel.hpp
    Models/CameraModel.cpp
    Models/DeviceModel.hpp
    Models/DeviceModel.cpp
    Models/UserModel.hpp
    Models/UserModel.cpp
    Models/AiImageModel.hpp
    Models/AiImageModel.cpp
    # Controllers
    Controllers/AlarmController.hpp
    Controllers/AlarmController.cpp
    Controllers/AppController.hpp
    Controllers/AppController.cpp
    Controllers/SettingsController.hpp
    Controllers/SettingsController.cpp
    # Services
    Services/AlarmService.hpp
    Services/AlarmService.cpp
    Services/ServerStatus.hpp
    Services/ServerStatus.cpp
    Services/VideoStream.hpp
    Services/VideoStream.cpp
)

target_compile_options(BackEnd PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

# @note Qt6 모듈 및 공통 유틸리티 라이브러리(AnoMapSTD) 연결
target_link_libraries(BackEnd PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Multimedia
    Qt6::GuiPrivate
    AnoMapSTD
)

target_include_directories(BackEnd PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src
)

# [Backend Workflow Guide]
# 1. 신규 UI 기능 필요 시 모델 또는 컨트롤러를 Qt/Back 하위 폴더에 작성
# 2. CMakeLists.txt 소스 목록에 등록 후 빌드 수행
# 3. Core::registerContextProperties() 함수에서 QML이 접근할 수 있도록 바인딩
