/**
 * @file main.cpp
 * @author Sang-In Lee (tkddls0614@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2023-06-22
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include "rtc/rtc.hpp"

#include "parse_cl.h"

#include <nlohmann/json.hpp>

// using namespace std;
using std::shared_ptr;
using std::weak_ptr;

template <class T>
weak_ptr<T> make_weak_ptr(shared_ptr<T> ptr) { return ptr; }

using nlohmann::json;

std::string localId;
std::unordered_map<std::string, shared_ptr<rtc::PeerConnection>> peerConnectionMap;
std::unordered_map<std::string, shared_ptr<rtc::DataChannel>> dataChannelMap;

shared_ptr<rtc::PeerConnection> createPeerConnection(const rtc::Configuration &config,
                                                     weak_ptr<rtc::WebSocket> wws,
                                                     std::string id);

int main(int argc, char *argv[]) {
    Cmdline params(argc, argv);

    rtc::InitLogger(rtc::LogLevel::Info);

    rtc::Configuration config;
    std::string stunServer = "";
    if (params.noStun()) {
		std::cout
		    << "No STUN server is configured. Only local hosts and public IP addresses supported."
		    << std::endl;
	} else {
		if (params.stunServer().substr(0, 5).compare("stun:") != 0) {
			stunServer = "stun:";
		}
		stunServer += params.stunServer() + ":" + std::to_string(params.stunPort());
		std::cout << "STUN server is " << stunServer << std::endl;
		config.iceServers.emplace_back(stunServer);
	}

	if (params.udpMux()) {
		std::cout << "ICE UDP mux enabled" << std::endl;
		config.enableIceUdpMux = true;
	}

	localId = "videoStreamSender";

    shared_ptr<rtc::WebSocket> ws = std::make_shared<rtc::WebSocket>();

    // Wait until webSocket created
    std::promise<void> wsPromise;
    std::future<void> wsFuture = wsPromise.get_future();

    ws->onOpen([&wsPromise]() {
        std::cout << "WebSocket connected, signaling ready" << std::endl;
        wsPromise.set_value(); // signal to wsFuture
    });

    ws->onError([&wsPromise](std::string s) {
        std::cout << "WebSocket error" << std::endl;
        wsPromise.set_exception(std::make_exception_ptr(std::runtime_error(s)));
        // signaling to wsFuture if exception (error) will happen
    });

    ws->onClosed([]() { std::cout << "WebSocket closed" << std::endl; });

    ws->onMessage([&config, wws = make_weak_ptr(ws)](auto data) {
        if (!std::holds_alternative<std::string>(data)) // if data is not string
            return;

        json message = json::parse(std::get<std::string>(data));

        auto it = message.find("id");
        if (it == message.end()) // if there's no "id"
            return;

        auto id = it->get<std::string>();

        it = message.find("type");
        if (it == message.end()) // if there's no "type"
            return;

        auto type = it->get<std::string>();

        shared_ptr<rtc::PeerConnection> pc;
        if (auto jt = peerConnectionMap.find(id); jt != peerConnectionMap.end()) {

        }

    });









    return 0;
}