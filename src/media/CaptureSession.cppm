export module media.CaptureSession;

import std;
import logger;

#ifdef __linux__
import media.Xdp;
using Backend = media::Xdp;
#else
import media.Win32;
using Backend = media::Win32;
#endif

export namespace media
{

struct CaptureSessionInfo
{
    std::string videoSource;
    std::string audioSource;
};

class CaptureSession
{
public:
    CaptureSession() = delete;

    static std::optional<CaptureSessionInfo> request()
    {
        logger::info("[CaptureSession] Requesting capture session.");

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
            .videoSource = std::format("d3d11screencapturesrc monitor-handle={} show-cursor=true", captureSession->monitorHandle),
            .audioSource = "wasapi2src loopback=true"
#endif
        };
    }
};

}
