#pragma once

#include <cstdint>
#include <optional>

namespace media::Win32Impl
{
    std::optional<std::uintptr_t> selectMonitor();
}
