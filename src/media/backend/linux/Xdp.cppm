module;

#include <gio/gio.h>
#include <gst/gst.h>

export module media.Backend;

import std;
import media.core.raii;

namespace media
{

class Xdp
{
private:
    struct ScreenCaptureSession
    {
        int pipewireFd{-1};
        std::uint32_t pipewireNodeId{0};
        core::raii::XdpSessionPtr session;

        ScreenCaptureSession() = default;
        ~ScreenCaptureSession();

        ScreenCaptureSession(const ScreenCaptureSession&) = delete;
        ScreenCaptureSession& operator=(const ScreenCaptureSession&) = delete;
        ScreenCaptureSession(ScreenCaptureSession&&) = delete;
        ScreenCaptureSession& operator=(ScreenCaptureSession&&) = delete;
    };

    struct Request
    {
        std::promise<bool> promise;
        std::unique_ptr<ScreenCaptureSession> capture;
        bool completed{false};

        void finish(bool success);
    };

    static void onStarted(GObject* source, GAsyncResult* result, gpointer userData);
    static void onCreated(GObject* source, GAsyncResult* result, gpointer userData);

public:
    [[nodiscard]] static std::unique_ptr<ScreenCaptureSession> requestScreenCapture();
};

export using Backend = Xdp;

}
