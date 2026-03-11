find_package(nlohmann_json REQUIRED)
find_package(boost_asio CONFIG REQUIRED)
find_package(boost_system CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)

# Pure C++ 정적 라이브러리 — Qt 링크 없음
add_library(AnoMapSTD STATIC
    Thread/ThreadPool.hpp
    Thread/ThreadPool.cpp
    Network/Header.hpp
    Network/INetworkService.hpp
    Network/NetworkService.hpp
    Network/NetworkService.cpp
    Network/Video.hpp
    Network/Video.cpp
    Network/Session.hpp
    Network/Session.cpp
    Domain/IStore.hpp
    Domain/Alarm.hpp
    Domain/Alarm.cpp
    Domain/Camera.hpp
    Domain/Camera.cpp
    Domain/Device.hpp
    Domain/Device.cpp
    Domain/ServerStatus.hpp
    Domain/ServerStatus.cpp
    Domain/User.hpp
    Config.hpp
)

target_include_directories(AnoMapSTD PRIVATE
    ${FFmpeg_INCLUDE_DIR}
)
target_include_directories(AnoMapSTD PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

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
)
