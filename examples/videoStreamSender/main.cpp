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
#include "play_video.h"
#include "config.h"

#include <nlohmann/json.hpp>

#include <thread>
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

int main(int argc, char *argv[]) try {
    Cmdline params(argc, argv);

    /* --- SET LOG & LOG LEVEL --- */
    rtc::InitLogger(rtc::LogLevel::Info);

    /* ------------------------------------------------------------- */
    /* -------------------- START configuration -------------------- */
    /* ------------------------------------------------------------- */
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
    /* ------------------------------------------------------------- */
    /* --------------------- END configuration --------------------- */
    /* ------------------------------------------------------------- */

    gst_init(&argc, &argv); // init gstreamer

#ifdef PLAY_VIDEO_NUM /* -------------------------------------- PLAY VIDEO NUM 1 */
    PlayVideo video_middle(VIDEO1);
    std::thread videoStreamThread_1(&PlayVideo::play, &video_middle);
    videoStreamThread_1.detach();
#if PLAY_VIDEO_NUM == 2 /* ------------------------------------ PLAY VIDEO NUM 2 */
    PlayVideo video_middle(VIDEO2);
    std::thread videoStreamThread_2(&PlayVideo::play, &video_middle);
    videoStreamThread_2.detach();
#if PLAY_VIDEO_NUM == 3 /* ------------------------------------ PLAY VIDEO NUM 3 */
    PlayVideo video_middle(VIDEO3);
    std::thread videoStreamThread_3(&PlayVideo::play, &video_middle);
    videoStreamThread_3.detach();
#endif /* ----------------------------------------------------- PLAY VIDEO NUM 3 */
#endif /* ----------------------------------------------------- PLAY VIDEO NUM 2 */
#endif /* ----------------------------------------------------- PLAY VIDEO NUM 1 */

    localId = LOCAL_ID; // Temporary local Id : CARNIVAL - can change if you want

    shared_ptr<rtc::WebSocket> ws = std::make_shared<rtc::WebSocket>(); // WebSocket to message signaling server

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

    ws->onMessage([&config, wws = make_weak_ptr(ws), &video_middle](auto data) {
        /* Check whether if data is string
            If data is not string, return  */
        if (!std::holds_alternative<std::string>(data)) 
            return;

        // Parse data(string) to json format
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
            /*
                if it was already peer connected before.
                ex) message type is answer or candidate
                in this case, it is candidate, NOT answer (it is vehicle side).
            */
            pc = jt->second;
        } else if (type == "offer") {
            /*
                If this peer receive peerconnection offering by other peer
                then create the peerconnection with received "Id"
                through WebSocket
            */
            pc = createPeerConnection(config, wws, id);

            // std::string sdp = message["sdp"].get<std::string>();
            // pc->setRemoteDescription(rtc::Description(sdp, type));

            // std::cout << "Answering to " + id << std::endl;

            // video_middle.addVideoMideaTrackOnPeerConnection(pc);

            // pc->setLocalDescription();

        } else  {
            return;
        }

        if (type == "offer") {
            auto sdp = message["sdp"].get<std::string>();

            video_middle.addVideoMideaTrackOnPeerConnection(pc);

            pc->setRemoteDescription(rtc::Description(sdp, type));

        } else if (type == "candidate") {
            auto sdp = message["candidate"].get<std::string>();
            auto mid = message["mid"].get<std::string>();
            pc->addRemoteCandidate(rtc::Candidate(sdp, mid));
        } // else -> We do not handle.

    });

    /*
        Making WebSocket URL to communicatie with signaling server
        Websocket is just created for communication with signaling server (master)
    */
    const std::string wsPrefix =
        params.webSocketServer().find("://") == std::string::npos ? "ws://" : "";
    const std::string url = wsPrefix + params.webSocketServer() + ":" +
                            std::to_string(params.webSocketPort()) + "/" + localId;

    std::cout << "WebSocket URL is " << url << std::endl;
    /*
        Try open WebSocket
        If the WebSocket is open, then "onOpen" callback is called
    */
    ws->open(url);

    std::cout << "Wating for signaling to be connected..." << std::endl;
    wsFuture.get(); // wait here until websocket "onOpen" callback is called

    /*
        below section is run only the WebSocket is opened
    */
    while (true) {
        // std::string remoteId;
        // std::cout << "Enter a remote ID to send offer: ";
        // std::cin >> remoteId;
        // std::cin.ignore();

        // if (remoteId.empty())
        //     break;

        // if (remoteId == localId) {
        //     std::cout << "Invalid remote ID (This is the local ID)" << std::endl;
        //     continue;
        // }

        // std::cout << "Offering to " + remoteId << std::endl;
        // shared_ptr<rtc::PeerConnection> pc = createPeerConnection(config, ws, remoteId);

        // // We are the offerer, so create a data channel to initiate the process
        // const std::string label = "This is the gstreamer react test";
        // std::cout << "Create DataChannel with label \"" << label << "\"" << std::endl;
        // shared_ptr<rtc::DataChannel> dc = pc->createDataChannel(label);

        // dc->onOpen([remoteId, wdc = make_weak_ptr(dc)]() {
        //     std::cout << "DataChannel from " << remoteId << " open" << std::endl;
        //     if (auto dataChannelPtr = wdc.lock())
        //         dataChannelPtr->send("Hello from " + localId);
        // });

        // dc->onClosed([remoteId]() { std::cout << "DataChannel from " << remoteId << " closed" << std::endl; });

        // dc->onMessage([remoteId, wdc = make_weak_ptr(dc)](auto data) {
        //     // data holds either std::string or rtc::binary
        //     if (std::holds_alternative<std::string>(data))
        //         std::cout << "Message from " << remoteId << " received: " << std::get<std::string>(data)
        //                   << std::endl;
        //     else
        //         std::cout << "Binary message from " << remoteId
        //                   << " received, size=" << std::get<rtc::binary>(data).size() << std::endl;
        // });

        // /* Hold and register dataChannel Connection member */
        // dataChannelMap.emplace(remoteId, dc);

        std::string quit;
        std::cout << "Quit? (q or Q)" << std::endl;
        std::cin >> quit;
        std::cin.ignore();

        if (quit == "Q" || quit == "q")
            break;
    } 

    std::cout << "Cleaning up..." << std::endl;

    dataChannelMap.clear();
    peerConnectionMap.clear();

    return 0;

} catch (const std::exception &e) {
    std::cout << "Error: " << e.what() << std::endl;
    dataChannelMap.clear();
    peerConnectionMap.clear();
    return -1;
}

/**
 * @brief Create a Peer Connection object based on config (with WebSocket)
 *         
 * @param config 
 * @param wws 
 * @param id 
 * @return shared_ptr<rtc::PeerConnection> 
 */
shared_ptr<rtc::PeerConnection> createPeerConnection(const rtc::Configuration &config,
                                                     weak_ptr<rtc::WebSocket> wws,
                                                     std::string id) {
    shared_ptr<rtc::PeerConnection> pc
        = std::make_shared<rtc::PeerConnection>(config);

    pc->onStateChange([](rtc::PeerConnection::State state) {
        std::cout << "State: " << state << std::endl;
    });

    pc->onGatheringStateChange([](rtc::PeerConnection::GatheringState state) {
        std::cout << "Gathering State: " << state << std::endl;
        if (state == rtc::PeerConnection::GatheringState::Complete) {
            connMx.lock();
            connected = true;
            connMx.unlock();
        }
    });

    pc->onLocalDescription([wws, id](rtc::Description description) {
        json message = {
            { "id", id },
            { "type", description.typeString() },
            { "sdp", std::string(description) }
        };
        if (auto webSocketPtr = wws.lock()) {
            webSocketPtr->send(message.dump());
        }
    });

    pc->onLocalCandidate([wws, id](rtc::Candidate candidate) {
        json message = {
            { "id", id },
            { "type", "candidate" },
            { "candidate", std::string(candidate) },
            { "mid", candidate.mid() }
        };
        if (auto webSocketPtr = wws.lock()) {
            webSocketPtr->send(message.dump());
        }
    });

    pc->onDataChannel([id](shared_ptr<rtc::DataChannel> dc) {
        std::cout << "DataChannel from" << id << " received with label \"" 
                  << dc->label() << "\"" << std::endl;

        dc->onOpen([wdc = make_weak_ptr(dc)]() {
            if (auto dataChannelPtr = wdc.lock())
                dataChannelPtr->send("Hello from " + localId);
        });

        dc->onClosed([id]() { std::cout << "DataChannel from " + id + "closed" << std::endl; });

        dc->onMessage([id](auto data) {
            // data holds either std::string or rtc::binary
            if (std::holds_alternative<std::string>(data))
                std::cout << "Message from " << id << " received: " << std::get<std::string>(data)
                          << std::endl;
            else
                std::cout << "Binary message from " << id 
                          << " received, size=" << std::get<rtc::binary>(data).size() << std::endl;
        });

        dataChannelMap.emplace(id, dc);
    });

    peerConnectionMap.emplace(id, pc);

    return pc;
}