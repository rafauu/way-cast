module;

#include <gst/gst.h>

export module media.VideoEncoder;

import std;
import logger;
import media.Raii;

static bool isElementAvailable(const char* elementName)
{
    media::Raii::GstElementFactoryPtr factory {gst_element_factory_find(elementName)};
    return factory != nullptr;
}

export namespace media
{

class VideoEncoder
{
private:
    static constexpr std::array EncoderPriorities{
        "nvh264enc",
#ifdef __linux__
        "vah264enc",
        "vah264lpenc",
#else
        "mfh264enc",
#endif
        "x264enc"
    };

public:
    VideoEncoder() = delete;

    static std::optional<std::string> getBestWithProperties()
    {
        return getBestEncoder().transform([](const auto& encoder) {
            return std::format("{} {}", encoder, getProperties(encoder));
        });
    }

private:
    static std::optional<std::string_view> getBestEncoder()
    {
        static auto availabilityList = [] {
            std::vector<std::string_view> available;
            logger::debug("[VideoEncoder] Availability:");

            for (auto encoder : EncoderPriorities)
            {
                const bool isAvailable = isElementAvailable(encoder);
                logger::debug("- {}: {}", encoder, isAvailable);
                if (isAvailable)
                {
                    available.emplace_back(encoder);
                }
            }
            return available;
        } ();

        static constexpr std::string_view overwrite = WAYCAST_ENCODER_OVERWRITE;

        if (not overwrite.empty())
        {
            logger::info("[VideoEncoder] Overwritten encoder: {}", overwrite);
            return overwrite;
        }

        if (not availabilityList.empty())
        {
            const auto bestEncoder = availabilityList.front();
            logger::debug("[VideoEncoder] Chosen encoder: {}", bestEncoder);
            return bestEncoder;
        }

        logger::error("[VideoEncoder] No available encoder");
        return std::nullopt;
    }

    static std::string_view getProperties(const auto& encoder)
    {
        if (encoder == "nvh264enc")
        {
            return "rc-mode=constqp qp-const-i=23 qp-const-p=23 preset=low-latency-hq gop-size=60";
        }
        else if (encoder == "mfh264enc")
        {
            return "rate-control=pcbr bitrate=4000000"; // tbd: adjust
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

}
