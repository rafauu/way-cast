module;

#include <csignal>

export module signals;

import std;
import logger;


static volatile std::sig_atomic_t signalRaised = 0;

export namespace signals
{

void registerHandlers()
{
    logger::info("[Signals] Registering handlers.");

    auto signalHandler = [] (int signal) noexcept {
        signalRaised = signal;
    };

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
}

bool isShutdownRequested()
{
    if (signalRaised == 0)
    {
        return false;
    }

    logger::info(
        "[Signals] Shutdown requested with signal {}.",
        static_cast<std::remove_volatile_t<decltype(signalRaised)>>(signalRaised)
    );
    return true;
}

}
