cmake_minimum_required(VERSION 3.7)
if(POLICY CMP0079)
	cmake_policy(SET CMP0079 NEW)
endif()

set(CLIENT_SOURCES
	main.cpp
	parse_cl.cpp
	parse_cl.h
)

add_executable(datachannel-client ${CLIENT_SOURCES})

set_target_properties(datachannel-client PROPERTIES
	CXX_STANDARD 17
	OUTPUT_NAME client)

find_package(Threads REQUIRED)
target_link_libraries(datachannel-client
	LibDataChannel::LibDataChannel
	Threads::Threads
	nlohmann_json::nlohmann_json
)
