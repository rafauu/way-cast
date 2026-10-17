module;

export module media.ScreenCapture;

import std;

#if defined(_WIN32)
import media.Wgc;
using Backend = media::Wgc;
#else
import media.XdgPortal;
using Backend = media::XdgPortal;
#endif

export namespace media
{

struct CaptureSessionInfo
{
    std::string videoSource;
    std::string audioSource;
};

namespace ScreenCapture
{
    std::optional<CaptureSessionInfo> requestSession()
    {
        const auto captureSession = Backend::requestScreenCapture();

        if (not captureSession)
        {
            return std::nullopt;
        }

        return CaptureSessionInfo{
#if defined(_WIN32)
            .videoSource = std::format("d3d11capturesrc window-handle={} cursor=true", captureSession->windowHandle),
            .audioSource = "wasapi2src loopback=true"
#else
            .videoSource = std::format("pipewiresrc fd={} path={} do-timestamp=true", captureSession->pipewire_fd, captureSession->pipewire_node_id),
            .audioSource = "pipewiresrc path=\"@DEFAULT_AUDIO_SINK@\" stream-properties=\"props,stream.capture.sink=true\" do-timestamp=true"
#endif
        };
    }
}

}
