module;

#include <cstdlib>

export module env;

import std;
import logger;


namespace
{

namespace impl
{
void setEnv(const char* name, const char* value)
{
#ifdef __linux__
    ::setenv(name, value, 1);
#else
    _putenv_s(name, value);
#endif
}

[[nodiscard]] constexpr char pathSeparator() noexcept
{
#ifdef __linux__
    return ':';
#else
    return ';';
#endif
}
}

void setEnv(const char* name, std::string value)
{
    if (value.empty())
    {
        return;
    }

    logger::info("[Environment] Setting {} to {}.", name, value);
    impl::setEnv(name, value.c_str());
}

void prependToEnv(const char* name, std::string value)
{
    if (value.empty())
    {
        return;
    }

    const char* oldValue = std::getenv(name);

    if (oldValue != nullptr and *oldValue != '\0')
    {
        value += impl::pathSeparator();
        value += oldValue;
    }

    logger::info("[Environment] Setting {} to {}.", name, value);
    impl::setEnv(name, value.c_str());
}

}

export namespace env
{

void configureRuntimePaths()
{
    prependToEnv("PATH",            WAYCAST_VCPKG_BIN);
    prependToEnv("GST_PLUGIN_PATH", WAYCAST_GSTREAMER_PLUGIN_PATH);
    prependToEnv("GST_PLUGIN_PATH", WAYCAST_GSTREAMER_RUST_PLUGIN_PATH);

    setEnv("GST_PLUGIN_SCANNER", WAYCAST_GSTREAMER_PLUGIN_SCANNER);
    setEnv("LIBVA_DRIVERS_PATH", WAYCAST_LIBVA_DRIVERS_PATH);
}

};
