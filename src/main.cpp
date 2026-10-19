import std;
import env;
import logger;
import media.CapturePipeline;
import signals;

int main()
{
    try
    {
        logger::Session _{};

        logger::info("==========================");
        logger::info("Native Screen-Share Engine");
        logger::info("==========================");

        env::configureRuntimePaths();
        signals::registerHandlers();

        {
            media::CapturePipeline _{};

            logger::info("[SERVER ACTIVE] WebRTCSink is running.");
            logger::info("[SERVER ACTIVE] Press Ctrl+C to shut down.");

            while (not signals::isShutdownRequested())
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
    catch (...)
    {
        logger::critical("[FATAL ERROR] Unknown non-standard exception caught.");
        return 1;
    }
}

