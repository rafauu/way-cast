#include <csignal>

import std;
import env;
import logger;
import media.CapturePipeline;

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
    logger::Session _{};

    logger::info("======================================================");
    logger::info("Native Screen-Share Engine (PipeWire + WebRTCSink)");
    logger::info("======================================================");

    env::setPaths();

    std::signal(SIGINT, requestShutdown);
    std::signal(SIGTERM, requestShutdown);

    try
    {
        {
            media::CapturePipeline _{};

            logger::info("[SERVER ACTIVE] WebRTCSink is running.");
            logger::info("[SERVER ACTIVE] Press Ctrl+C to shut down.");

            while (shutdownRequested == 0)
            {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(250ms);
            }

            logger::info("\n[Shutdown] Signal received. Stopping...");
        }

        logger::info("[Shutdown] Capture pipeline stopped safely.");
        return 0;
    }
    catch (const std::exception& e)
    {
        logger::critical("[FATAL ERROR] {}", e.what());
        return 1;
    }
}

