#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <print>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

import net_server;
import hls_engine;

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using tcp = asio::ip::tcp;

namespace HlsCast::Network {


asio::awaitable<void> send_html(tcp::socket& socket, const http::request<http::string_body>& req) {
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::content_type, "text/html");

    std::ifstream file("index.html");
    if (file) {
        std::stringstream ss;
        ss << file.rdbuf();
        res.body() = ss.str();
    } else {
        res.result(http::status::not_found);
        res.body() = "Error: index.html not found on server disk.";
    }

    res.prepare_payload();
    co_await http::async_write(socket, res, asio::use_awaitable);
}

    asio::awaitable<void> send_binary(tcp::socket& socket,
                                 const http::request<http::string_body>& req,
                                 const std::vector<uint8_t>& data,
                                 std::string_view mime_type) {
    // We use vector_body because it avoids copying the data into a string
    http::response<http::vector_body<uint8_t>> res{http::status::ok, req.version()};
    res.set(http::field::content_type, mime_type);
    res.set(http::field::server, "HlsCast-Gemini");
    res.set(http::field::access_control_allow_origin, "*"); // Allow cross-origin for HLS players
    res.body() = data;
    res.prepare_payload();

    co_await http::async_write(socket, res, asio::use_awaitable);
}

    asio::awaitable<void> handle_session(tcp::socket socket, HlsCast::HlsManager& hls) {
         beast::flat_buffer buffer;
         try {
             for (;;) {
                 http::request<http::string_body> req;
                 co_await http::async_read(socket, buffer, req, asio::use_awaitable);

                 auto target = req.target();
                 if (target == "/") {
                     co_await send_html(socket, req);
                 }
                 else if (target == "/playlist.m3u8") {
                     auto pl = hls.get_playlist();
                     http::response<http::string_body> res{http::status::ok, req.version()};
                     res.set(http::field::content_type, "application/vnd.apple.mpegurl");
                     res.body() = pl ? *pl : "";
                     res.prepare_payload();
                     co_await http::async_write(socket, res, asio::use_awaitable);
                 }
                 else if (target.starts_with("/segment_")) {
                     std::string target(req.target());
                     int id = std::stoi(target.substr(9, target.find(".ts")));

                     auto data = hls.get_segment(id);
                     if (data)
                         co_await send_binary(socket, req, *data, "video/mp2t");
                 }

                 if (!req.keep_alive()) break;
             }
         } catch (...) {}
     }

    asio::awaitable<void> listener(uint16_t port, HlsCast::HlsManager& hls) {
        auto executor = co_await asio::this_coro::executor;
        tcp::acceptor acceptor(executor, {tcp::v4(), port});
        for (;;) {
            tcp::socket socket = co_await acceptor.async_accept(asio::use_awaitable);
            asio::co_spawn(executor, handle_session(std::move(socket), hls), asio::detached);
        }
    }

    void Server::run(std::stop_token st) {
        asio::io_context ioc;
        asio::co_spawn(ioc, listener(m_port, m_hls), asio::detached);

        while (!st.stop_requested()) {
            ioc.run_one_for(std::chrono::milliseconds(50));
        }
    }
}
