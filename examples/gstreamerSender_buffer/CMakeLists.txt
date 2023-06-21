cmake_minimum_required(VERSION 3.7)
if (POLICY CMP0097)
  cmake_policy(SET CMP0097 NEW)
endif()

# find_package()
find_package(PkgConfig REQUIRED)
pkg_check_modules(gst REQUIRED IMPORTED_TARGET gstreamer-base-1.0)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${gst_INCLUDE_DIR})

add_executable(gstreamerSender_buffer main.cpp)

# add_backward(gstreamerClient)
target_link_libraries(gstreamerSender_buffer PkgConfig::gst)