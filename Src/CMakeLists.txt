cmake_minimum_required(VERSION 3.16)

project(PureCpp)

find_package(nlohmann_json REQUIRED)
find_package(boost_asio CONFIG REQUIRED)
find_package(boost_system CONFIG REQUIRED)
qt_add_library(AnoMapSTD STATIC
    Thread/ThreadPool.hpp
    Thread/ThreadPool.cpp
    Network/Header.hpp
    Network/Video.hpp
    Network/Video.cpp
    Network/Session.hpp
    Network/Session.cpp
)

# FFmpeg configuration (assuming downloaded manually in project root)
set(FFMPEG_DIR "${CMAKE_SOURCE_DIR}/ffmpeg-master-latest-win64-gpl-shared")

target_include_directories(AnoMapSTD PRIVATE
    ${FFMPEG_DIR}/include
)

target_link_libraries(AnoMapSTD PRIVATE
    nlohmann_json::nlohmann_json
    Boost::asio
    Boost::system
    ${FFMPEG_DIR}/lib/avcodec.lib
    ${FFMPEG_DIR}/lib/avformat.lib
    ${FFMPEG_DIR}/lib/avutil.lib
    ${FFMPEG_DIR}/lib/swscale.lib
)
