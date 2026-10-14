module;

#include <gst/gst.h>

export module GstPipelineManager;

import GstRaii;
import WaylandPortal;
import std;

export class GstPipelineManager
{
public:
    GstPipelineManager()
    {
        gst_init(nullptr, nullptr);

        mediaThread = std::jthread(
            [this](std::stop_token stopToken) {
                mediaLoop(stopToken);
            }
        );
    }

    ~GstPipelineManager() = default;

    GstPipelineManager(const GstPipelineManager&) = delete;
    GstPipelineManager& operator=(const GstPipelineManager&) = delete;

    GstPipelineManager(GstPipelineManager&&) = delete;
    GstPipelineManager& operator=(GstPipelineManager&&) = delete;

private:
    std::jthread mediaThread;

    void mediaLoop(std::stop_token stopToken)
    {
        auto portalSession = WaylandPortal::requestScreenCapture();

        if (not portalSession)
        {
            std::println(
                std::cerr,
                "[GStreamer] Failed to obtain PipeWire "
                "screen-capture session."
            );
            return;
        }

        GstRaii::GstPipelineElementPtr pipeline = buildPipeline(
            portalSession->pipewire_fd,
            portalSession->pipewire_node_id
        );

        if (not pipeline) {
            std::println(
                std::cerr,
                "[GStreamer] Pipeline creation failed."
            );
            return;
        }

        GstRaii::GMainLoopPtr mainLoop {g_main_loop_new(nullptr, FALSE)};

        if (not mainLoop)
        {
            std::println(
                std::cerr,
                "[GStreamer] Failed to create GLib main loop."
            );
            return;
        }

        const GstStateChangeReturn state =
            gst_element_set_state(
                pipeline.get(),
                GST_STATE_PLAYING
            );

        if (state == GST_STATE_CHANGE_FAILURE)
        {
            std::println(
                std::cerr,
                "[GStreamer] Failed to set pipeline to PLAYING."
            );
            return;
        }

        std::println("[GStreamer] Screen-sharing pipeline started.");

        std::stop_callback stopCallback(
            stopToken,
            [loop = mainLoop.get()] {
                g_main_loop_quit(loop);
            }
        );

        if (not stopToken.stop_requested())
        {
            g_main_loop_run(mainLoop.get());
        }

        std::println("[GStreamer] Pipeline resources released.");
    }

    GstRaii::GstPipelineElementPtr buildPipeline(
        int pipewireFd,
        unsigned int pipewireNodeId
    )
    {
        const std::string pipelineDescription =
            std::format(
                "pipewiresrc fd={} path={} do-timestamp=true "
                "! queue max-size-buffers=2 max-size-bytes=0 max-size-time=0 leaky=downstream "
                "! videoconvert "
                "! videoscale "
                "! video/x-raw,format=NV12,width=1920,height=1080 "
                "! queue max-size-buffers=2 max-size-bytes=0 max-size-time=0 leaky=downstream "
              #if WAYCAST_NVIDIA_OVER_VAAPI
                "! nvh264enc rc-mode=constqp qp-const-i=23 qp-const-p=23 preset=low-latency-hq gop-size=60 "
              #else
                "! vah264lpenc rate-control=cqp qpi=23 qpp=23 cabac=false dct8x8=false b-frames=0 key-int-max=60 "
              #endif
                "! queue max-size-buffers=2 max-size-bytes=0 max-size-time=0 leaky=downstream "
                "! h264parse config-interval=-1 "
                "! video/x-h264,profile=constrained-baseline,stream-format=avc,alignment=au "
                "! queue max-size-buffers=2 max-size-bytes=0 max-size-time=0 leaky=downstream "
                "! webrtcsink "
                "run-signalling-server=true "
                "signalling-server-host=0.0.0.0 "
                "signalling-server-port=8443 "
                "run-web-server=true "
                "web-server-host-addr=http://0.0.0.0:8080/ "
                "web-server-directory={} "
                "video-caps=video/x-h264 "
                "start-bitrate=6000000 "
                "min-bitrate=2000000 "
                "max-bitrate=12000000 "
                "meta=\"meta,name=waycast\"",
                pipewireFd,
                pipewireNodeId,
                WAYCAST_WEB_DIR
            );

        std::println("[GStreamer] Pipeline:\n{}", pipelineDescription);

        GstRaii::GErrorPtr error;

        GstRaii::GstPipelineElementPtr pipeline {gst_parse_launch(
            pipelineDescription.c_str(),
            std::out_ptr(error)
        )};

        if (error)
        {
            std::println(
                std::cerr,
                "[GStreamer] Failed to create pipeline: {}",
                error->message
            );

            return nullptr;
        }

        if (not pipeline)
        {
            std::println(
                std::cerr,
                "[GStreamer] gst_parse_launch returned nullptr."
            );

            return nullptr;
        }

        return pipeline;
    }
};

