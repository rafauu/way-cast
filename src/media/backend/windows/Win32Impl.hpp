#pragma once

#include <cstdint>
#include <optional>

namespace media::Win32Impl
{
    [[nodiscard]] std::optional<std::uintptr_t> selectMonitor();
}
