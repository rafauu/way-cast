module;

#include <gst/gst.h>

export module GstPipelineManager;

import WaylandPortal;
import std;

#ifndef VCPKG_GST_PLUGIN_DIR
#define VCPKG_GST_PLUGIN_DIR ""
#endif

static gboolean bus_callback(
    GstBus*,
    GstMessage* msg,
    gpointer
) {
    switch (GST_MESSAGE_TYPE(msg)) {

    case GST_MESSAGE_ERROR: {
        GError* err = nullptr;
        gchar* debug_info = nullptr;

        gst_message_parse_error(msg, &err, &debug_info);

        std::println(
            std::cerr,
            "[GStreamer ERROR] Source: {} | Message: {}",
            GST_OBJECT_NAME(msg->src),
            err ? err->message : "unknown"
        );

        if (debug_info) {
            std::println(
                std::cerr,
                "[GStreamer Debug] {}",
                debug_info
            );
        }

        g_clear_error(&err);
        g_free(debug_info);
        break;
    }

    case GST_MESSAGE_WARNING: {
        GError* err = nullptr;
        gchar* debug_info = nullptr;

        gst_message_parse_warning(msg, &err, &debug_info);

        std::println(
            std::cerr,
            "[GStreamer WARNING] Source: {} | Message: {}",
            GST_OBJECT_NAME(msg->src),
            err ? err->message : "unknown"
        );

        if (debug_info) {
            std::println(
                std::cerr,
                "[GStreamer Debug] {}",
                debug_info
            );
        }

        g_clear_error(&err);
        g_free(debug_info);
        break;
    }

    case GST_MESSAGE_STATE_CHANGED: {
        if (GST_IS_PIPELINE(msg->src)) {
            GstState old_state;
            GstState new_state;
            GstState pending_state;

            gst_message_parse_state_changed(
                msg,
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


export class GstPipelineManager {
public:
    GstPipelineManager() = default;

    ~GstPipelineManager() {
        stop();
    }

    void start() {
        if (running_)
            return;

        /*
         * Make sure GStreamer can find:
         *
         *   libgstpipewire.so
         *   libgstrswebrtc.so
         *
         * from the vcpkg installation.
         */
        if (std::getenv("GST_PLUGIN_PATH") == nullptr &&
            std::string_view(VCPKG_GST_PLUGIN_DIR).size() > 0) {

            ::setenv(
                "GST_PLUGIN_PATH",
                VCPKG_GST_PLUGIN_DIR,
                1
            );
        }

        gst_init(nullptr, nullptr);

        /*
         * Optional sanity checks.
         */
        GstElementFactory* pipewire_factory =
            gst_element_factory_find("pipewiresrc");

        if (!pipewire_factory) {
            std::println(
                std::cerr,
                "[GStreamer] pipewiresrc was not found."
            );
        } else {
            std::println(
                "[GStreamer] Found pipewiresrc."
            );

            gst_object_unref(pipewire_factory);
        }

        GstElementFactory* webrtcsink_factory =
            gst_element_factory_find("webrtcsink");

        if (!webrtcsink_factory) {
            std::println(
                std::cerr,
                "[GStreamer] webrtcsink was not found."
            );
        } else {
            std::println(
                "[GStreamer] Found webrtcsink."
            );

            gst_object_unref(webrtcsink_factory);
        }

        running_ = true;

        media_thread_ = std::thread([this] {
            mediaLoop();
        });
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

        /*
         * GStreamer bus watch requires a GLib main context.
         */
        GMainLoop* main_loop =
            g_main_loop_new(nullptr, FALSE);

        GstBus* bus =
            gst_element_get_bus(pipeline_);

        gst_bus_add_watch(
            bus,
            bus_callback,
            nullptr
        );

        gst_object_unref(bus);

        /*
         * Run GLib event processing.
         */
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

        /*
         * request_stop() invokes the callback,
         * which quits the GLib main loop.
         */

        g_main_loop_unref(main_loop);
    }

void buildPipeline() {
    auto portal_session =
        WaylandPortal::requestScreenCapture();

    if (!portal_session) {
        std::println(
            std::cerr,
            "[GStreamer] Failed to obtain PipeWire screen-capture session."
        );
        return;
    }

    const auto& session = *portal_session;

    const std::string pipewire_path =
        session.pipewire_serial != 0
        ? std::to_string(session.pipewire_serial)
        : std::to_string(session.pipewire_node_id);

    const std::string pipeline_description = std::format(
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
    "/home/rafal/way-cast/vcpkg/packages/"
    "gst-plugin-webrtc_x64-linux-dynamic/share/"
    "gstwebrtc-api/dist"
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
        error = nullptr;

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

