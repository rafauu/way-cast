#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/use_awaitable.hpp>

import AppConfig;
import GstPipelineManager;
import HttpWebServer;
import MpscQueue;
import std;

namespace net = boost::asio;

int main() {
    std::println("==================================================================");
    std::println(" C++26 Wayland Native Screen-Share Engine (PipeWire + MPSC Bus)   ");
    std::println("==================================================================");

    try {
        LockFreeMpscQueue<OutboundMessage> outbound_queue;

        auto pipeline_mgr = std::make_shared<GstPipelineManager>(outbound_queue);
        pipeline_mgr->start();

        web::HttpWebServer server(std::string(config::HOST_ADDRESS), config::PORT, pipeline_mgr);
        server.start(config::THREAD_POOL_SIZE);

        net::io_context signal_ioc;
        net::signal_set signals(signal_ioc, SIGINT, SIGTERM);

        net::co_spawn(signal_ioc, [&]() -> net::awaitable<void> {
            int sig = co_await signals.async_wait(net::use_awaitable);
            std::println("\n[Shutdown] Signal {} received. Terminating gracefully...", sig);

            server.stop();
            pipeline_mgr->stop();
        }, net::detached);

        std::println("[SERVER ACTIVE] WebServer listening on http://localhost:8000");
        std::println("[SERVER ACTIVE] Press Ctrl+C to shut down cleanly.\n");

        signal_ioc.run();

        std::println("[Shutdown] All background pools & pipelines stopped safely.");
    }
    catch (const std::exception& e) {
        std::println(std::cerr, "[FATAL ERROR] {}", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
