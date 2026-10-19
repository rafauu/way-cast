module;

#include <csignal>

export module signals;

import std;
import logger;


static volatile std::sig_atomic_t shutdownRequested = 0;

export namespace signals
{

void registerHandlers()
{
    logger::info("[Signals] Registering handlers.");

    auto signalHandler = [] (int) noexcept {
        shutdownRequested = 1;
    };

    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);
}

bool isShutdownRequested()
{
    return shutdownRequested != 0;
}

}
