cmake_minimum_required(VERSION 3.0.0)
project(gstreamerReceiver
    VERSION 0.1.0
    LANGUAGES C CXX
)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(gst REQUIRED IMPORTED_TARGET gstreamer-base-1.0)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${gst_INCLUDE_DIR})

add_executable(gstreamerReceiver main.cpp)

target_link_libraries(gstreamerReceiver PkgConfig::gst)