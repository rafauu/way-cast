module;

export module ScreenCapture;

import std;

#if defined(_WIN32)
import Wgc;
using Backend = Wgc;
#else
import XdgPortal;
using Backend = XdgPortal;
#endif

export struct CaptureSessionInfo
{
    std::string videoSource;
    std::string audioSource;
};

export namespace ScreenCapture
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
