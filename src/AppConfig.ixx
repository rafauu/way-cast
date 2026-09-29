export module AppConfig;

import std;

export namespace config {
    constexpr std::string_view HOST_ADDRESS = "0.0.0.0";
    constexpr std::uint16_t PORT = 8000;
    constexpr std::size_t THREAD_POOL_SIZE = 4;
}
