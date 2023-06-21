cmake_minimum_required(VERSION 3.7.0)
if (POLICY CMP0097)
  cmake_policy(SET CMP0097 NEW)
endif()

set(VIDEO_STREAM_SENDER_SRC
  main.cpp
)

# Link libraries
# find_package(PkgConfig REQUIRED)
# pkg_check_modules(gst REQUIRED IMPORTED_TARGET gstreamer-base-1.0)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${gst_INCLUDE_DIR})
include_directories(./include)

add_executable(video-stream-sender ${VIDEO_STREAM_SENDER_SRC})

set_target_properties(video-stream-sender PROPERTIES
  CXX_STANDARD  17
  OUTPUT_NAME   videoStreamSender
)

target_link_libraries(video-stream-sender
  PkgConfig::gst
  LibDataChannel::LibDataChannel
  Threads::Threads
  nlohmann_json::nlohmann_json
)
