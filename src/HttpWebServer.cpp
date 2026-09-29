module;

#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <optional>
#include <boost/asio.hpp>
#include <boost/beast.hpp>

module HttpWebServer;

import GstPipelineManager;
import std;

namespace web {

struct HttpWebServer::Impl {
    std::string address_str;
    unsigned short port;
    std::shared_ptr<GstPipelineManager> pipeline_mgr;
    boost::asio::io_context ioc;
    std::optional<boost::asio::ip::tcp::acceptor> acceptor;
    std::vector<std::thread> worker_threads;

    Impl(std::string addr, unsigned short p, std::shared_ptr<GstPipelineManager> pm)
        : address_str(std::move(addr)), port(p), pipeline_mgr(std::move(pm)) {}

    void do_accept() {
        namespace net = boost::asio;
        namespace beast = boost::beast;
        namespace websocket = beast::websocket;
        using tcp = boost::asio::ip::tcp;

        acceptor->async_accept(boost::asio::make_strand(ioc), [this](beast::error_code ec, tcp::socket socket) {
            if (!ec) {
                std::thread([s = std::move(socket), pm = pipeline_mgr]() mutable {
                    try {
                        namespace http = boost::beast::http;
                        beast::flat_buffer buffer;

                        // 1. Read the initial HTTP request headers
                        http::request<http::string_body> req;
                        http::read(s, buffer, req);

                        // 2. Check if the client is asking for a WebSocket upgrade
                        if (websocket::is_upgrade(req)) {
                            websocket::stream<tcp::socket> ws{std::move(s)};
                            ws.accept(req);

                            pm->setActiveSession(1);

                            // Immediately send the initial Offer that was pushed to the queue
                            if (auto outbound = pm->popOutbound()) {
                                ws.write(net::buffer(outbound->json_payload));
                                std::println("[WebServer] Sent SDP Offer to browser.");
                            }

                            // Enter read loop for Answer and ICE candidates
                            for (;;) {
                                beast::flat_buffer read_buffer;
                                ws.read(read_buffer);
                                std::string msg = beast::buffers_to_string(read_buffer.data());

                                if (msg.contains("\"type\":\"answer\"")) {
                                    pm->postInbound(InboundEvent{
                                        .type = InboundEvent::Type::RemoteAnswer,
                                        .session_id = 1,
                                        .sdp_or_candidate = msg
                                    });
                                } else if (msg.contains("\"type\":\"candidate\"")) {
                                    pm->postInbound(InboundEvent{
                                        .type = InboundEvent::Type::RemoteCandidate,
                                        .session_id = 1,
                                        .sdp_or_candidate = msg,
                                        .mline_index = 0
                                    });
                                }
                            }
                        } else {
                            // 3. Handle normal HTTP requests (serve HTML frontend)
                            http::response<http::string_body> res{http::status::ok, req.version()};
                            res.set(http::field::server, "WayCast Server");
                            res.set(http::field::content_type, "text/html");
                            res.body() = R"(
                                <!DOCTYPE html>
                                <html lang="en">
                                <head>
                                    <meta charset="UTF-8">
                                    <meta name="viewport" content="width=device-width, initial-scale=1.0">
                                    <title>C++26 Wayland Screen Share</title>
                                    <style>
                                        body {
                                            margin: 0;
                                            background-color: #121212;
                                            color: #ffffff;
                                            font-family: sans-serif;
                                            display: flex;
                                            flex-direction: column;
                                            align-items: center;
                                            justify-content: center;
                                            height: 100vh;
                                        }
                                        h2 {
                                            margin-bottom: 10px;
                                            font-weight: 400;
                                            color: #b0b0b0;
                                        }
                                        .video-container {
                                            position: relative;
                                        }
                                        video {
                                            width: 80vw;
                                            max-height: 80vh;
                                            background-color: #000000;
                                            border: 1px solid #333;
                                            border-radius: 8px;
                                            box-shadow: 0 4px 20px rgba(0,0,0,0.5);
                                            display: block;
                                        }
                                        #play-overlay {
                                            position: absolute;
                                            top: 0; left: 0; width: 100%; height: 100%;
                                            background: rgba(0,0,0,0.7);
                                            display: flex;
                                            align-items: center;
                                            justify-content: center;
                                            border-radius: 8px;
                                            cursor: pointer;
                                            font-size: 20px;
                                            font-weight: bold;
                                            color: #fff;
                                            transition: background 0.3s;
                                        }
                                        #play-overlay:hover {
                                            background: rgba(0,0,0,0.5);
                                        }
                                        #status {
                                            margin-top: 15px;
                                            font-size: 14px;
                                            color: #888;
                                        }
                                    </style>
                                </head>
                                <body>
                                    <h2>C++26 Screen Share Stream</h2>
                                    <div class="video-container">
                                        <video id="video" autoplay playsinline muted></video>
                                        <div id="play-overlay">Click to Start Stream</div>
                                    </div>
                                    <div id="status">Connecting to signaling server...</div>

                                    <script>
                                        const statusDiv = document.getElementById('status');
                                        const videoElement = document.getElementById('video');
                                        const playOverlay = document.getElementById('play-overlay');

                                        const pc = new RTCPeerConnection({
                                            iceServers: [{ urls: 'stun:stun.l.google.com:19302' }]
                                        });
                                        window.pc = pc;

                                        pc.ontrack = (event) => {
                                            console.log("Received remote track:", event.streams[0]);
                                            statusDiv.innerText = "Stream active! Click overlay to watch.";
                                            statusDiv.style.color = "#4caf50";
                                            videoElement.srcObject = event.streams[0];
                                        };

                                        // Explicit user gesture handler to bypass autoplay blocks
                                        playOverlay.addEventListener('click', async () => {
                                            console.log("Overlay clicked!");
                                            try {
                                                await videoElement.play();
                                                playOverlay.style.display = 'none';
                                                console.log("Video playback started successfully.");
                                            } catch (err) {
                                                console.error("Playback failed:", err);
                                                statusDiv.innerText = "Playback error: " + err.message;
                                            }
                                        });

                                        // Monitor decoder/media errors
                                        videoElement.addEventListener('error', (e) => {
                                            console.error("Video element error code:", videoElement.error ? videoElement.error.code : 'unknown');
                                        });

                                        pc.onicecandidate = (event) => {
                                            if (event.candidate) {
                                                ws.send(JSON.stringify({
                                                    type: 'candidate',
                                                    candidate: event.candidate.candidate,
                                                    sdpMLineIndex: event.candidate.sdpMLineIndex
                                                }));
                                            }
                                        };

                                        const wsProtocol = window.location.protocol === 'https:' ? 'wss://' : 'ws://';
                                        const ws = new WebSocket(wsProtocol + window.location.host);

                                        ws.onopen = () => {
                                            statusDiv.innerText = "Waiting for stream offer...";
                                        };

                                        ws.onmessage = async (event) => {
                                            try {
                                                const msg = JSON.parse(event.data);
                                                if (msg.type === 'offer') {
                                                    statusDiv.innerText = "Processing stream offer...";
                                                    await pc.setRemoteDescription(new RTCSessionDescription({
                                                        type: 'offer',
                                                        sdp: msg.sdp
                                                    }));

                                                    const answer = await pc.createAnswer();
                                                    await pc.setLocalDescription(answer);

                                                    ws.send(JSON.stringify({
                                                        type: 'answer',
                                                        sdp: answer.sdp
                                                    }));
                                                    statusDiv.innerText = "Establishing P2P media link...";
                                                }
                                                else if (msg.type === 'candidate') {
                                                    if (msg.candidate) {
                                                        await pc.addIceCandidate(new RTCIceCandidate({
                                                            candidate: msg.candidate.candidate,
                                                            sdpMLineIndex: msg.candidate.sdpMLineIndex
                                                        }));
                                                    }
                                                }
                                            } catch (err) {
                                                console.error("Signaling error:", err);
                                            }
                                        };
                                    </script>
                                </body>
                                </html>
                                )";
                            res.prepare_payload();
                            http::write(s, res);
                        }
                    } catch (const std::exception&) {
                        // Handle socket disconnects cleanly
                    }
                }).detach();
            }
            do_accept();
        });
    }

    void run(std::size_t thread_count) {
        auto const address = boost::asio::ip::make_address(address_str);
        boost::asio::ip::tcp::endpoint endpoint{address, port};

        acceptor.emplace(ioc, endpoint);
        do_accept();

        for (std::size_t i = 0; i < thread_count; ++i) {
            worker_threads.emplace_back([this] { ioc.run(); });
        }
    }
};

HttpWebServer::HttpWebServer(std::string address, unsigned short port, std::shared_ptr<GstPipelineManager> pipeline_mgr)
    : pimpl_(std::make_unique<Impl>(std::move(address), port, std::move(pipeline_mgr))) {}

HttpWebServer::~HttpWebServer() = default;

HttpWebServer::HttpWebServer(HttpWebServer&&) noexcept = default;
HttpWebServer& HttpWebServer::operator=(HttpWebServer&&) noexcept = default;

void HttpWebServer::start(std::size_t threads) {
    if (pimpl_) {
        pimpl_->run(threads);
    }
}

void HttpWebServer::stop() {
    if (pimpl_) {
        pimpl_->ioc.stop();
        for (auto& t : pimpl_->worker_threads) {
            if (t.joinable()) t.join();
        }
    }
}

} // namespace web
