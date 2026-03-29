# @file Src/CMakeLists.txt
# @brief 프로젝트 핵심 비즈니스 로직 라이브러리 (AnoMapSTD) 구성 파일
#
# 이 파일은 플랫폼 독립적인 핵심 로직(네트워킹, 도메인 모델, 스레드 풀, 암호화)을 관리합니다.
# Qt 의존성이 전혀 없는 순수 C++ 라이브러리이며, nlohmann_json, Boost.Asio, OpenSSL 등을 사용합니다.

find_package(nlohmann_json REQUIRED)
find_package(boost_asio CONFIG REQUIRED)
find_package(boost_system CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(libyuv CONFIG REQUIRED)
find_package(PalSigslot CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
# @brief AnoMapSTD 정적 라이브러리 생성
# 
# **표준 사용 방법론 (Standard Usage Methodology):**
# 1. **통신 규격 정의**: Network/Protocol.hpp에서 메시지 구문 분석 정의
# 2. **데이터 저장소 구현**: Domain 폴더에 IStore를 상속받는 전용 Store 추가
# 3. **서비스 통합**: NetworkManager + sigslot 기반으로 패킷 처리 및 도메인 업데이트
add_library(AnoMapSTD STATIC
    ## Network
    Network/NetworkProtocol.hpp
    Network/NetworkManager.hpp
    Network/NetworkManager.cpp
    Network/NetworkEngine.hpp
    Network/NetworkEngine.cpp
    Network/VideoEngine.hpp
    Network/VideoEngine.cpp

    ## Domain
    Domain/CameraManager.hpp
    Domain/CameraManager.cpp
    Domain/CameraBridge.hpp
    Domain/CameraBridge.cpp
    Domain/AlarmManager.hpp
    Domain/AlarmManager.cpp
    Domain/AlarmBridge.hpp
    Domain/StatusManager.hpp
    Domain/StatusManager.cpp
    Domain/AuthBridge.hpp
    Domain/AuthBridge.cpp
    Domain/AuthManager.hpp

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
