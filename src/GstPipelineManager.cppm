module;

#include <gst/gst.h>

export module GstPipelineManager;

import WaylandPortal;
import std;

#ifndef WAYCAST_GSTREAMER_PLUGIN_DIR
#define WAYCAST_GSTREAMER_PLUGIN_DIR ""
#endif

#ifndef WAYCAST_GSTREAMER_RUST_PLUGIN_DIR
#define WAYCAST_GSTREAMER_RUST_PLUGIN_DIR ""
#endif

#ifndef WAYCAST_WEB_DIR
#define WAYCAST_WEB_DIR ""
#endif

namespace {

gboolean busCallback(
    GstBus*,
    GstMessage* message,
    gpointer
) {
    switch (GST_MESSAGE_TYPE(message)) {

    case GST_MESSAGE_ERROR: {
        GError* error = nullptr;
        gchar* debug = nullptr;

        gst_message_parse_error(
            message,
            &error,
            &debug
        );

        std::println(
            std::cerr,
            "[GStreamer ERROR] Source: {} | Message: {}",
            GST_OBJECT_NAME(message->src),
            error ? error->message : "unknown"
        );

        if (debug) {
            std::println(
                std::cerr,
                "[GStreamer Debug] {}",
                debug
            );
        }

        g_clear_error(&error);
        g_free(debug);

        break;
    }

    case GST_MESSAGE_WARNING: {
        GError* error = nullptr;
        gchar* debug = nullptr;

        gst_message_parse_warning(
            message,
            &error,
            &debug
        );

        std::println(
            std::cerr,
            "[GStreamer WARNING] Source: {} | Message: {}",
            GST_OBJECT_NAME(message->src),
            error ? error->message : "unknown"
        );

        if (debug) {
            std::println(
                std::cerr,
                "[GStreamer Debug] {}",
                debug
            );
        }

        g_clear_error(&error);
        g_free(debug);

        break;
    }

    case GST_MESSAGE_STATE_CHANGED: {
        if (GST_IS_PIPELINE(message->src)) {
            GstState old_state;
            GstState new_state;
            GstState pending_state;

            gst_message_parse_state_changed(
                message,
                &old_state,
                &new_state,
                &pending_state
            );

            std::println(
                "[GStreamer Pipeline State] {} -> {}",
                gst_element_state_get_name(old_state),
                gst_element_state_get_name(new_state)
            );
        }

        break;
    }

    default:
        break;
    }

    return TRUE;
}

void checkElementFactory(std::string_view name) {
    GstElementFactory* factory =
        gst_element_factory_find(
            std::string{name}.c_str()
        );

    if (!factory) {
        std::println(
            std::cerr,
            "[GStreamer] {} was not found.",
            name
        );

        return;
    }

    std::println(
        "[GStreamer] Found {}.",
        name
    );

    gst_object_unref(factory);
}

} // namespace


export class GstPipelineManager {
public:
    GstPipelineManager() = default;

    ~GstPipelineManager() {
        stop();
    }

    void start() {
        if (running_)
            return;

        const std::string plugin_path =
            std::string(WAYCAST_GSTREAMER_PLUGIN_DIR) +
            ":" +
            std::string(WAYCAST_GSTREAMER_RUST_PLUGIN_DIR);

        if (!plugin_path.empty()) {
            ::setenv(
                "GST_PLUGIN_PATH",
                plugin_path.c_str(),
                1
            );
        }

        gst_init(nullptr, nullptr);

        checkElementFactory("pipewiresrc");
        checkElementFactory("webrtcsink");

        running_ = true;

        media_thread_ = std::thread(
            [this] {
                mediaLoop();
            }
        );
    }

    void stop() {
        if (!running_.exchange(false))
            return;

        if (pipeline_) {
            gst_element_set_state(
                pipeline_,
                GST_STATE_NULL
            );
        }

        if (media_thread_.joinable())
            media_thread_.join();

        if (pipeline_) {
            gst_object_unref(pipeline_);
            pipeline_ = nullptr;
        }
    }

private:
    std::atomic<bool> running_{false};

    GstElement* pipeline_{nullptr};

    std::thread media_thread_;

    void mediaLoop() {
        buildPipeline();

        if (!pipeline_) {
            std::println(
                std::cerr,
                "[GStreamer] Pipeline creation failed."
            );

            running_ = false;
            return;
        }

        GMainLoop* main_loop =
            g_main_loop_new(
                nullptr,
                FALSE
            );

        GstBus* bus =
            gst_element_get_bus(pipeline_);

        gst_bus_add_watch(
            bus,
            busCallback,
            nullptr
        );

        gst_object_unref(bus);

        std::jthread glib_thread(
            [main_loop](std::stop_token stop_token) {

                std::stop_callback stop_callback(
                    stop_token,
                    [main_loop] {
                        g_main_loop_quit(main_loop);
                    }
                );

                g_main_loop_run(main_loop);
            }
        );

        while (running_) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(50)
            );
        }

        glib_thread.request_stop();

        g_main_loop_unref(main_loop);
    }

    void buildPipeline() {
        auto portal_session =
            WaylandPortal::requestScreenCapture();

        if (!portal_session) {
            std::println(
                std::cerr,
                "[GStreamer] Failed to obtain PipeWire "
                "screen-capture session."
            );

            return;
        }

        const auto& session = *portal_session;

        /*
         * The portal may provide a PipeWire serial.
         * Some portals don't, so fall back to the node ID.
         */
        const std::string pipewire_path =
            session.pipewire_serial != 0
                ? std::to_string(session.pipewire_serial)
                : std::to_string(session.pipewire_node_id);

        const std::string pipeline_description =
            std::format(
                "pipewiresrc "
                "fd={} "
                "path={} "
                "do-timestamp=true "
                "! queue "
                "max-size-buffers=1 "
                "max-size-bytes=0 "
                "max-size-time=0 "
                "leaky=downstream "
                "! videoconvert "
                "! webrtcsink "
                "name=screen "
                "run-signalling-server=true "
                "signalling-server-host=0.0.0.0 "
                "signalling-server-port=8443 "
                "run-web-server=true "
                "web-server-host-addr=http://0.0.0.0:8080/ "
                "web-server-directory={} "
                "video-caps=video/x-vp8 "
                "start-bitrate=12000000 "
                "min-bitrate=6000000 "
                "max-bitrate=20000000 "
                "meta=\"meta,name=waycast\"",
                session.pipewire_fd,
                pipewire_path,
                WAYCAST_WEB_DIR
            );

        std::println(
            "[GStreamer] Pipeline:\n{}",
            pipeline_description
        );

        GError* error = nullptr;

        pipeline_ = gst_parse_launch(
            pipeline_description.c_str(),
            &error
        );

        if (error) {
            std::println(
                std::cerr,
                "[GStreamer] Failed to create pipeline: {}",
                error->message
            );

            g_error_free(error);

            if (pipeline_) {
                gst_object_unref(pipeline_);
                pipeline_ = nullptr;
            }

            return;
        }

        if (!pipeline_) {
            std::println(
                std::cerr,
                "[GStreamer] gst_parse_launch returned nullptr."
            );

            return;
        }

        const GstStateChangeReturn state =
            gst_element_set_state(
                pipeline_,
                GST_STATE_PLAYING
            );

        if (state == GST_STATE_CHANGE_FAILURE) {
            std::println(
                std::cerr,
                "[GStreamer] Failed to set pipeline to PLAYING."
            );

            return;
        }

        std::println(
            "[GStreamer] Screen sharing pipeline started."
        );
    }
};

