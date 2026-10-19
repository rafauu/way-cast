module;

#include <libportal/portal.h>
#include <libportal/session.h>
#include <unistd.h>

export module media.Xdp;

import std;
import logger;
import media.Raii;

export namespace media
{

class Xdp
{
public:
    struct CaptureSession
    {
        int pipewireFd{-1};
        uint32_t pipewireNodeId{0};
        Raii::XdpSessionPtr session;

        CaptureSession() = default;

        CaptureSession(const CaptureSession&) = delete;
        CaptureSession& operator=(const CaptureSession&) = delete;
        CaptureSession(CaptureSession&&) = delete;
        CaptureSession& operator=(CaptureSession&&) = delete;

        ~CaptureSession()
        {
            if (pipewireFd >= 0)
            {
                close(pipewireFd);
            }
        }
    };

private:
    struct Request
    {
        std::promise<bool> promise;
        std::unique_ptr<CaptureSession> capture;
        bool completed{false};

        void finish(bool success)
        {
            if (std::exchange(completed, true))
            {
                return;
            }

            promise.set_value(success);
        }
    };

    static void onStarted(GObject* source, GAsyncResult* result, gpointer userData)
    {
        auto* request = static_cast<Request*>(userData);
        GError* error = nullptr;

        if (not xdp_session_start_finish(XDP_SESSION(source), result, &error))
        {
            logger::error(
                "[Xdp] Failed to start session: {}",
                error ? error->message : "unknown error"
            );

            g_clear_error(&error);
            request->finish(false);
            return;
        }

        g_autoptr(GVariant) streams = xdp_session_get_streams(request->capture->session.get());

        if (not streams or g_variant_n_children(streams) == 0)
        {
            logger::error("[Xdp] Portal returned no streams.");
            request->finish(false);
            return;
        }

        g_autoptr(GVariant) properties = nullptr;

        g_variant_get_child(
            streams,
            0,
            "(u@a{sv})",
            &request->capture->pipewireNodeId,
            &properties
        );

        if (request->capture->pipewireNodeId == 0)
        {
            logger::error("[Xdp] Invalid PipeWire node ID.");
            request->finish(false);
            return;
        }

        request->capture->pipewireFd = xdp_session_open_pipewire_remote(request->capture->session.get());

        if (request->capture->pipewireFd < 0)
        {
            logger::error("[Xdp] Failed to open PipeWire remote.");
            request->finish(false);
            return;
        }

        logger::info(
            "[Xdp] Capture ready: node={}, fd={}",
            request->capture->pipewireNodeId,
            request->capture->pipewireFd
        );

        request->finish(true);
    }

    static void onCreated(GObject* source, GAsyncResult* result, gpointer userData)
    {
        auto* request = static_cast<Request*>(userData);
        GError* error = nullptr;

        XdpSession* session = xdp_portal_create_screencast_session_finish(XDP_PORTAL(source), result, &error);

        if (not session)
        {
            logger::error(
                "[Xdp] Failed to create session: {}",
                error ? error->message : "unknown error"
            );

            g_clear_error(&error);
            request->finish(false);
            return;
        }

        g_clear_error(&error);
        request->capture->session.reset(session);

        xdp_session_start(
            request->capture->session.get(),
            nullptr,
            nullptr,
            onStarted,
            request
        );
    }

public:
    static std::unique_ptr<CaptureSession> requestScreenCapture()
    {
        g_autoptr(XdpPortal) portal = xdp_portal_new();

        if (not portal)
        {
            logger::error("[Xdp] Failed to create portal.");
            return nullptr;
        }

        Raii::GMainLoopPtr loop {g_main_loop_new(nullptr, FALSE)};

        if (not loop)
        {
            logger::error("[Xdp] Failed to create GLib main loop.");
            return nullptr;
        }

        Request request;
        request.capture = std::make_unique<CaptureSession>();
        auto future = request.promise.get_future();

        xdp_portal_create_screencast_session(
            portal,
            static_cast<XdpOutputType>(XDP_OUTPUT_MONITOR | XDP_OUTPUT_WINDOW),
            XDP_SCREENCAST_FLAG_NONE,
            XDP_CURSOR_MODE_EMBEDDED,
            XDP_PERSIST_MODE_NONE,
            nullptr,
            nullptr,
            onCreated,
            &request
        );

        std::jthread loopThread([&loop] {
            g_main_loop_run(loop.get());
        });

        if (future.wait_for(std::chrono::seconds(10)) == std::future_status::timeout)
        {
            logger::error("[Xdp] Timed out waiting for portal.");

            g_main_loop_quit(loop.get());
            loopThread.join();
            return nullptr;
        }

        const bool success = future.get();

        g_main_loop_quit(loop.get());
        loopThread.join();

        if (not success)
        {
            return nullptr;
        }

        return std::move(request.capture);
    }
};

}
