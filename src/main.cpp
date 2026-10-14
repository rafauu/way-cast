#include <csignal>

import Environment;
import GstPipelineManager;
import Logger;
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
    Logger::Session logSession{};

    Logger::info("==============================================================");
    Logger::info("Wayland Native Screen-Share Engine (PipeWire + WebRTCSink)");
    Logger::info("==============================================================");

    Environment::setPaths();

    std::signal(SIGINT, requestShutdown);
    std::signal(SIGTERM, requestShutdown);

    try
    {
        {
            GstPipelineManager pipelineManager{};

            Logger::info("[SERVER ACTIVE] WebRTCSink is running.");
            Logger::info("[SERVER ACTIVE] Press Ctrl+C to shut down.");

            while (shutdownRequested == 0)
            {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(100ms);
            }

            Logger::info("\n[Shutdown] Signal received. Stopping...");
        }

        Logger::info("[Shutdown] GStreamer stopped safely.");
        return 0;
    }
    catch (const std::exception& e)
    {
        Logger::critical("[FATAL ERROR] {}", e.what());
        return 1;
    }
}

