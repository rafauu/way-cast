module;

#include <cstdlib>

export module env;

import std;
import logger;


static void setEnvIfUnset(
    const char* name,
    std::string_view value
)
{
    if (std::getenv(name) == nullptr and not value.empty())
    {
        logger::info("[Environment] Setting {} to {}.", name, value.data());
        ::setenv(name, value.data(), 1);
    }
}

export namespace env
{

void configureRuntimePaths()
{
    setEnvIfUnset("GST_PLUGIN_SCANNER", WAYCAST_GSTREAMER_PLUGIN_SCANNER);
    setEnvIfUnset("GST_PLUGIN_PATH",    WAYCAST_GSTREAMER_PLUGIN_PATH);
    setEnvIfUnset("LIBVA_DRIVERS_PATH", WAYCAST_LIBVA_DRIVERS_PATH);
}

};
