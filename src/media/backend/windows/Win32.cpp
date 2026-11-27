module;

#include "Win32Impl.hpp"
#include <cstdint>
#include <optional>

module media.Backend;

import logger;


namespace media
{

std::optional<Win32::SessionData> Win32::requestScreenCapture()
{
    logger::info("[Win32] Opening monitor selection dialog.");

    const auto monitor = win32::selectMonitor();

    if (not monitor)
    {
        logger::warning("[Win32] Monitor selection cancelled.");
        return std::nullopt;
    }

    return Win32::SessionData{
        .monitorHandle = *monitor
    };
}

}
