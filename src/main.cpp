#include <csignal>

import GstPipelineManager;
import std;

namespace
{

std::condition_variable_any shutdown_cv;
std::mutex shutdown_mutex;
std::atomic_bool shutdown_requested{false};

void request_shutdown(int)
{
    shutdown_requested.store(true, std::memory_order_release);
    shutdown_cv.notify_all();
}

}

int main()
{
    std::println(
        "=================================================================="
    );
    std::println(
        " C++26 Wayland Native Screen-Share Engine (PipeWire + WebRTCSink)"
    );
    std::println(
        "=================================================================="
    );

    try
    {
        std::signal(SIGINT, request_shutdown);
        std::signal(SIGTERM, request_shutdown);

        {
            GstPipelineManager pipelineManager{};

            std::println("[SERVER ACTIVE] WebRTCSink is running.");
            std::println("[SERVER ACTIVE] Press Ctrl+C to shut down.\n");

            std::mutex wait_mutex;
            std::unique_lock lock(wait_mutex);

            shutdown_cv.wait(
                lock,
                [] {
                    return shutdown_requested.load(
                        std::memory_order_acquire
                    );
                }
            );

            std::println("\n[Shutdown] Signal received. Stopping...");
        }

        std::println("[Shutdown] GStreamer stopped safely.");
        return 0;
    }
    catch (const std::exception& e)
    {
        std::println(std::cerr, "[FATAL ERROR] {}", e.what());
        return 1;
    }
}

