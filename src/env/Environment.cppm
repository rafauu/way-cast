module;

#include <cstdlib>

export module env;

import std;
import logger;


static void setEnv(const char* name, const char* value)
{
#ifdef __linux__
    ::setenv(name, value, 1);
#else
    _putenv_s(name, value);
#endif
}

static void setEnvIfUnset(const char* name, std::string_view value)
{
    if (std::getenv(name) == nullptr and not value.empty())
    {
        logger::info("[Environment] Setting {} to {}.", name, value.data());
        setEnv(name, value.data());
    }
}

static void appendToEnv(const char* name, std::string_view value)
{
    if (value.empty())
    {
        return;
    }

    const char* oldValue = std::getenv(name);

    std::string newValue{value};

    if (oldValue != nullptr and *oldValue != '\0')
    {
#ifdef _WIN32
        constexpr char separator = ';';
#else
        constexpr char separator = ':';
#endif
        newValue += separator;
        newValue += oldValue;
    }

    logger::info("[Environment] Setting {} to {}.", name, newValue);

    setEnv(name, newValue.c_str());
}


export namespace env
{

void configureRuntimePaths()
{
    appendToEnv("PATH",            WAYCAST_VCPKG_BIN);
    appendToEnv("GST_PLUGIN_PATH", WAYCAST_GSTREAMER_PLUGIN_PATH);
    appendToEnv("GST_PLUGIN_PATH", WAYCAST_GSTREAMER_RUST_PLUGIN_PATH);

    setEnvIfUnset("GST_PLUGIN_SCANNER", WAYCAST_GSTREAMER_PLUGIN_SCANNER);
    setEnvIfUnset("LIBVA_DRIVERS_PATH", WAYCAST_LIBVA_DRIVERS_PATH);
}

};
