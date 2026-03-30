find_package(nlohmann_json REQUIRED)
find_package(boost_asio CONFIG REQUIRED)
find_package(boost_system CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(libyuv CONFIG REQUIRED)
find_package(PalSigslot CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)

add_library(AnoMapSTD STATIC
    ## Network
    Network/NetworkProtocol.hpp
    Network/Manager/NetworkManager.hpp
    Network/Manager/NetworkManager.cpp
    Network/Engine/NetworkEngine.hpp
    Network/Engine/NetworkEngine.cpp
    Network/Engine/VideoEngine.hpp
    Network/Engine/VideoEngine.cpp

    ## Domain
    Domain/Manager/CameraManager.hpp
    Domain/Manager/CameraManager.cpp
    Domain/Manager/AlarmManager.hpp
    Domain/Manager/AlarmManager.cpp
    Domain/Manager/StatusManager.hpp
    Domain/Manager/StatusManager.cpp
    Domain/Manager/AuthManager.hpp
    Domain/CameraBridge.hpp
    Domain/CameraBridge.cpp
    Domain/AlarmBridge.hpp
    Domain/AuthBridge.hpp
    Domain/AuthBridge.cpp

    ## Crypt
    Crypt/DtlsEngine.hpp
    Crypt/DtlsEngine.cpp
    Crypt/TlsEngine.hpp
    Crypt/TlsEngine.cpp

    ## Thread
    Thread/ThreadEngine.hpp
    Thread/ThreadEngine.cpp

    ## Config
    Config.hpp
)

# @note 외부 라이브러리(OpenSSL, FFmpeg 등)의 인클루드 경로 설정
target_include_directories(AnoMapSTD PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${FFmpeg_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
)

target_compile_options(AnoMapSTD PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
)

# @note 의존성 라이브러리 링크 (Boost, OpenSSL, FFmpeg, yuv 등)
target_link_libraries(AnoMapSTD PUBLIC
    nlohmann_json::nlohmann_json
    Boost::asio
    Boost::system
    OpenSSL::SSL
    OpenSSL::Crypto
    FFmpeg::avcodec
    FFmpeg::avformat
    FFmpeg::avutil
    FFmpeg::swscale
    FFmpeg::swresample
    yuv
    Pal::Sigslot
    yaml-cpp::yaml-cpp
)

# [AnoMapSTD Workflow Guide]
# 1. 네트워크 통신 흐름: TextConnect가 수신한 패킷을 NetworkManager::Packets()로 전달, MediaConnect가 UDP 이미지 수신
# 2. 데이터 업데이트: NetworkManager는 sigslot 시그널로 Bridge에 전달, Bridge가 Manager 업데이트
# 3. 스레드 관리: 비동기 작업은 ThreadPool을 통해 Worker 스레드에서 수행하도록 구현
