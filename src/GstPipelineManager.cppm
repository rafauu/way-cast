module;

#include <gst/gst.h>

export module GstPipelineManager;

import GstRaii;
import Logger;
import WaylandPortal;
import std;

export class GstPipelineManager
{
public:
    GstPipelineManager()
    {
        gst_init(nullptr, nullptr);

        Logger::info("[GStreamer] Initializing pipeline manager.");

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
        Logger::info("[GStreamer] Requesting Wayland screen-capture session.");

        auto portalSession = WaylandPortal::requestScreenCapture();

        if (not portalSession)
        {
            Logger::error(
                "[GStreamer] Failed to obtain PipeWire screen-capture session."
            );
            return;
        }

        GstRaii::GstPipelineElementPtr pipeline = buildPipeline(
            portalSession->pipewire_fd,
            portalSession->pipewire_node_id
        );

        if (not pipeline)
        {
            Logger::error("[GStreamer] Pipeline creation failed.");
            return;
        }

        GstRaii::GMainLoopPtr mainLoop{
            g_main_loop_new(nullptr, FALSE)
        };

        if (not mainLoop)
        {
            Logger::error("[GStreamer] Failed to create GLib main loop.");
            return;
        }

        const GstStateChangeReturn state =
            gst_element_set_state(
                pipeline.get(),
                GST_STATE_PLAYING
            );

        if (state == GST_STATE_CHANGE_FAILURE)
        {
            Logger::error(
                "[GStreamer] Failed to set pipeline to PLAYING."
            );
            return;
        }

        Logger::info("[GStreamer] Screen-sharing pipeline started.");

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

        Logger::info("[GStreamer] Pipeline resources released.");
    }

    GstRaii::GstPipelineElementPtr buildPipeline(
        int pipewireFd,
        unsigned int pipewireNodeId
    )
    {
        const std::string pipelineDescription =
            std::format(
                "pipewiresrc fd={} path={} do-timestamp=true "
                "! queue max-size-buffers=1 max-size-bytes=0 max-size-time=0 leaky=downstream "
                "! videoconvert "
                "! videoscale "
                "! video/x-raw,format=NV12 "
                "! queue max-size-buffers=1 max-size-bytes=0 max-size-time=0 leaky=downstream "
              #if WAYCAST_NVIDIA_OVER_VAAPI
                "! nvh264enc rc-mode=constqp qp-const-i=23 qp-const-p=23 preset=low-latency-hq gop-size=60 "
              #else
                "! vah264lpenc rate-control=cqp qpi=23 qpp=23 cabac=false dct8x8=false b-frames=0 key-int-max=60 "
              #endif
                "! h264parse config-interval=-1 "
                "! video/x-h264,profile=constrained-baseline,stream-format=avc,alignment=au "
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

        Logger::debug(
            "[GStreamer] Pipeline description:\n{}",
            pipelineDescription
        );

        GstRaii::GErrorPtr error;

        GstRaii::GstPipelineElementPtr pipeline{
            gst_parse_launch(
                pipelineDescription.c_str(),
                std::out_ptr(error)
            )
        };

        if (error)
        {
            Logger::error(
                "[GStreamer] Failed to create pipeline: {}",
                error->message
            );

            return nullptr;
        }

        if (not pipeline)
        {
            Logger::error(
                "[GStreamer] gst_parse_launch returned nullptr."
            );

            return nullptr;
        }

        return pipeline;
    }
};
