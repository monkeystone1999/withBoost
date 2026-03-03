cmake_minimum_required(VERSION 3.16)

project(PureCpp)

find_package(nlohmann_json REQUIRED)
find_package(boost_asio CONFIG REQUIRED)
find_package(boost_system CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
qt_add_library(AnoMapSTD STATIC
    Thread/ThreadPool.hpp
    Thread/ThreadPool.cpp
    Network/Header.hpp
    Network/Video.hpp
    Network/Video.cpp
    Network/Session.hpp
    Network/Session.cpp
)

target_include_directories(AnoMapSTD PRIVATE
    ${FFmpeg_INCLUDE_DIR}
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
)
