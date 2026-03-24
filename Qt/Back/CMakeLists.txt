# @file Qt/Back/CMakeLists.txt
# @brief Qt 기반 백엔드 로직 라이브러리 (BackEnd) 구성 파일
#
# 이 파일은 QML 레이어와 통신하는 C++ 모델 및 컨트롤러들을 관리합니다.
# 핵심 디자인 패턴으로 MVVM 구조의 ViewModel 역할을 수행하며,
# UI(Qt/Front)와 비즈니스 로직(Src) 사이의 가교 역할을 합니다.

# @brief BackEnd 정적 라이브러리 생성
# 
# **표준 사용 방법론 (Standard Usage Methodology):**
# 1. **구성 요소 추가**: 신규 모델/컨트롤러 생성 시 이 파일의 소스 목록에 추가
# 2. **인스턴스 관리**: Core.cpp에서 해당 클래스를 인스턴스화하고 관리
# 3. **QML 노출**: setContextProperty()를 통해 QML 컨텍스트에 프로퍼티로 등록
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
    Services/AlarmManager.hpp
    Services/AlarmManager.cpp
    Services/NetworkBridge.hpp
    Services/NetworkBridge.cpp
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
