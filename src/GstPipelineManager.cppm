module;

#include <gst/gst.h>

export module GstPipelineManager;

import WaylandPortal;
import std;

static void setEnvIfUnset(
    const char* name,
    std::string_view value
)
{
    if (::std::getenv(name) == nullptr && !value.empty())
    {
        ::setenv(name, value.data(), 1);
    }
}

export class GstPipelineManager
{
public:
    GstPipelineManager()
    {
        setEnvIfUnset("GST_PLUGIN_PATH", WAYCAST_GSTREAMER_PLUGIN_DIR);
        setEnvIfUnset("LIBVA_DRIVERS_PATH", WAYCAST_LIBVA_DRIVERS_PATH);

        gst_init(nullptr, nullptr);

        running_ = true;

        media_thread_ = std::jthread(
            [this] {
                mediaLoop();
            }
        );
    }

    ~GstPipelineManager()
    {
        running_ = false;

        if (pipeline_)
        {
            gst_element_set_state(pipeline_, GST_STATE_NULL);
        }

        media_thread_.request_stop();

        if (media_thread_.joinable())
        {
            media_thread_.request_stop();
            media_thread_.join();
        }

        if (pipeline_)
        {
            gst_object_unref(pipeline_);
            pipeline_ = nullptr;
        }
    }


private:
    std::atomic<bool> running_{false};

    GstElement* pipeline_{nullptr};

    std::jthread media_thread_;

    void mediaLoop()
    {
        buildPipeline();

        if (!pipeline_) {
            std::println(
                std::cerr,
                "[GStreamer] Pipeline creation failed."
            );

            running_ = false;
            return;
        }

        GMainLoop* main_loop = g_main_loop_new(nullptr, FALSE);

        std::stop_callback stop_callback(
            media_thread_.get_stop_token(),
            [main_loop]
            {
                g_main_loop_quit(main_loop);
            }
        );

        g_main_loop_run(main_loop);

        g_main_loop_unref(main_loop);
    }

    void buildPipeline()
    {
        auto portal_session = WaylandPortal::requestScreenCapture();

        if (!portal_session) {
            std::println(
                std::cerr,
                "[GStreamer] Failed to obtain PipeWire "
                "screen-capture session."
            );
            return;
        }

        const std::string pipeline_description =
            std::format(
                "pipewiresrc fd={} path={} do-timestamp=true "
                "! queue max-size-buffers=1 max-size-bytes=0 max-size-time=0 leaky=downstream "
                "! videoconvert "
                "! video/x-raw,format=NV12 "
                "! vah264lpenc rate-control=cqp qpi=23 qpp=23 cabac=false dct8x8=false b-frames=0 "
                "! h264parse "
                "! video/x-h264,profile=constrained-baseline,stream-format=avc,alignment=au "
                "! webrtcsink "
                "name=screen "
                "run-signalling-server=true "
                "signalling-server-host=0.0.0.0 "
                "signalling-server-port=8443 "
                "run-web-server=true "
                "web-server-host-addr=http://0.0.0.0:8080/ "
                "web-server-directory={} "
                "video-caps=video/x-h264 "
                "start-bitrate=8000000 "
                "min-bitrate=2000000 "
                "max-bitrate=12000000 "
                "meta=\"meta,name=waycast\"",
                portal_session->pipewire_fd,
                portal_session->pipewire_node_id,
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

