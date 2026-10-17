module;

#include <gst/gst.h>

export module VideoEncoders;

import Logger;
import MediaRaii;
import std;

static bool isElementAvailable(const char* elementName)
{
    MediaRaii::GstElementFactoryPtr factory {gst_element_factory_find(elementName)};
    return factory != nullptr;
}

export class VideoEncoders
{
private:
    static constexpr std::array ENCODERS_PRIORITIES{
        "nvh264enc",
        "vah264enc",
        "vah264lpenc",
        "x264enc"
    };

public:
    VideoEncoders() = delete;

    static void printAvailability()
    {
        Logger::debug("[VideoEncoders] Available encoders:");

        for (auto encoder : ENCODERS_PRIORITIES)
        {
            Logger::debug("- {}: {}", encoder, isElementAvailable(encoder));
        }
    }

    static std::optional<std::string> getBestWithProperties()
    {
        return getBestEncoder().transform([](const auto& encoder) {
            return std::format("{} {}", encoder, getProperties(encoder));
        });
    }

private:
    static std::optional<std::string_view> getBestEncoder()
    {
        static constexpr std::string_view overwrite = WAYCAST_ENCODER_OVERWRITE;

        if (not overwrite.empty())
        {
            Logger::info("[VideoEncoders] Overwritten encoder: {}", overwrite);
            return overwrite;
        }

        auto it = std::ranges::find_if(ENCODERS_PRIORITIES, isElementAvailable);

        if (it != ENCODERS_PRIORITIES.end())
        {
            Logger::debug("[VideoEncoders] Chosen encoder: {}", *it);
            return *it;
        }

        Logger::error("[VideoEncoders] No available encoders");
        return std::nullopt;
    }

    static std::string_view getProperties(const auto& encoder)
    {
        if (encoder == "nvh264enc")
        {
            return "rc-mode=constqp qp-const-i=23 qp-const-p=23 preset=low-latency-hq gop-size=60";
        }
        else if (encoder == "vah264enc" or encoder == "vah264lpenc")
        {
            return "rate-control=cqp qpi=23 qpp=23 cabac=false dct8x8=false b-frames=0 key-int-max=60";
        }
        else if (encoder == "x264enc")
        {
            return "tune=zerolatency speed-preset=ultrafast pass=quant quantizer=23 bframes=0 key-int-max=60 cabac=false sliced-threads=true";
        }
        else
        {
            std::unreachable();
        }
    }
};
