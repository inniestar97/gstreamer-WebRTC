cmake_minimum_required(VERSION 3.7)
project(gstreamerSender
    VERSION 0.1.0
    LANGUAGES C CXX
)
find_package(PkgConfig REQUIRED)
pkg_check_modules(gst REQUIRED IMPORTED_TARGET gstreamer-base-1.0)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${gst_INCLUDE_DIR})

add_executable(gstreamerSender main.cpp)

target_link_libraries(gstreamerSender PkgConfig::gst)