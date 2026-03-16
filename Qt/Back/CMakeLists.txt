
qt_add_library(BackEnd STATIC
    # ── 인증 ────────────────────────────────────────────────────────────────
    Auth.cpp
    Auth.hpp
    # ── 카메라 모델 ─────────────────────────────────────────────────────────
    Camera.hpp
    Camera.cpp
    # ── 디바이스 모델 ───────────────────────────────────────────────────────
    Device.hpp
    Device.cpp
    # ── 네트워크 브릿지 ─────────────────────────────────────────────────────
    NetworkBridge.hpp NetworkBridge.cpp
    ServerStatus.hpp ServerStatus.cpp
    AiImageModel.hpp AiImageModel.cpp
    # ── 알람 ────────────────────────────────────────────────────────────────
    AlarmManager.hpp
    AlarmManager.cpp
    AlarmController.hpp
    AlarmController.cpp
    # ── 앱 전역 컨트롤러 (신규) ──────────────────────────────────────────────
    AppController.hpp
    AppController.cpp
    # ── 설정 컨트롤러 (신규) ────────────────────────────────────────────────
    SettingsController.hpp
    SettingsController.cpp
    # ── 비디오 파이프라인 ────────────────────────────────────────────────────
    VideoStream.hpp
    VideoStream.cpp
    # ── 서버 상태 ────────────────────────────────────────────────────────────
    ServerStatus.hpp
    ServerStatus.cpp
    # ── 유저 모델 ────────────────────────────────────────────────────────────
    UserModel.cpp
    UserModel.hpp
)


target_compile_options(BackEnd PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

target_link_libraries(BackEnd PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Multimedia
    Qt6::GuiPrivate
    AnoMapSTD
)


target_include_directories(BackEnd PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src
)
