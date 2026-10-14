#include <csignal>

import GstPipelineManager;
import Environment;
import std;

namespace
{

volatile std::sig_atomic_t shutdownRequested = 0;

void requestShutdown(int) noexcept
{
    shutdownRequested = 1;
}

}

int main()
{
    std::println(
        "=================================================================="
    );
    std::println(
        " Wayland Native Screen-Share Engine (PipeWire + WebRTCSink)"
    );
    std::println(
        "=================================================================="
    );

    try
    {
        std::signal(SIGINT, requestShutdown);
        std::signal(SIGTERM, requestShutdown);

        Environment::setPaths();

        {
            GstPipelineManager pipelineManager{};

            std::println("[SERVER ACTIVE] WebRTCSink is running.");
            std::println("[SERVER ACTIVE] Press Ctrl+C to shut down.\n");

            while (shutdownRequested == 0)
            {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(100ms);
            }

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

