module;

#include <cstdlib>

export module Environment;

import Logger;
import std;


static void setEnvIfUnset(
    const char* name,
    std::string_view value
)
{
    if (::std::getenv(name) == nullptr and not value.empty())
    {
        Logger::info("[Environment] Setting {} to {}.", name, value.data());
        ::setenv(name, value.data(), 1);
    }
}

export class Environment
{
public:
    static void setPaths()
    {
        setEnvIfUnset("GST_PLUGIN_PATH", WAYCAST_GSTREAMER_PLUGIN_DIR);
        setEnvIfUnset("LIBVA_DRIVERS_PATH", WAYCAST_LIBVA_DRIVERS_PATH);
    }
};
