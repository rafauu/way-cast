module;

export module media.ScreenCapture;

import std;
import logger;

#ifdef __linux__
import media.Xdp;
using Backend = media::Xdp;
#else
import media.Wgc;
using Backend = media::Wgc;
#endif

export namespace media
{

struct CaptureSessionInfo
{
    std::string videoSource;
    std::string audioSource;
};

class ScreenCapture
{
public:
    static std::optional<CaptureSessionInfo> requestSession()
    {
        logger::info("[ScreenCapture] Requesting screen-capture session.");

        static const auto captureSession = Backend::requestScreenCapture();

        if (not captureSession)
        {
            return std::nullopt;
        }

        return CaptureSessionInfo{
#ifdef __linux__
            .videoSource = std::format("pipewiresrc fd={} path={} do-timestamp=true", captureSession->pipewireFd, captureSession->pipewireNodeId),
            .audioSource = "pipewiresrc path=\"@DEFAULT_AUDIO_SINK@\" stream-properties=\"props,stream.capture.sink=true\" do-timestamp=true"
#else
            .videoSource = std::format("d3d11capturesrc window-handle={} cursor=true", captureSession->windowHandle),
            .audioSource = "wasapi2src loopback=true"
#endif
        };
    }
};

}
