export module media.CaptureSession;

import std;
import logger;
import media.Backend;


export namespace media
{

class CaptureSession
{
public:
    struct Info
    {
        std::string videoSource;
        std::string audioSource;
    };

    CaptureSession() = delete;

    [[nodiscard]] static std::optional<Info> request()
    {
        logger::info("[CaptureSession] Requesting capture session.");

        static const auto captureSession = Backend::requestScreenCapture();

        if (not captureSession)
        {
            return std::nullopt;
        }

        return Info{
#ifdef __linux__
            .videoSource = std::format("pipewiresrc fd={} path={} do-timestamp=true", captureSession->pipewireFd, captureSession->pipewireNodeId),
            .audioSource = "pipewiresrc path=\"@DEFAULT_AUDIO_SINK@\" stream-properties=\"props,stream.capture.sink=true\" do-timestamp=true"
#else
            .videoSource = std::format("d3d11screencapturesrc monitor-handle={} show-cursor=true", captureSession->monitorHandle),
            .audioSource = "wasapi2src loopback=true"
#endif
        };
    }
};

}
