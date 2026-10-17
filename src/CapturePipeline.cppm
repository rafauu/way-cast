module;

#include <gst/gst.h>

export module CapturePipeline;

import Logger;
import MediaRaii;
import ScreenCapture;
import VideoEncoders;
import std;

export class CapturePipeline
{
public:
    CapturePipeline()
    {
        Logger::info("[CapturePipeline] Initializing capture pipeline.");

        gst_init(nullptr, nullptr);

        VideoEncoders::printAvailability();

        Logger::info("[CapturePipeline] Requesting screen-capture session.");

        auto sessionInfo = ScreenCapture::requestSession();

        if (not sessionInfo)
        {
            Logger::error(
                "[CapturePipeline] Failed to obtain PipeWire screen-capture session."
            );
            throw std::runtime_error("Failed to obtain PipeWire screen-capture session.");
        }

        auto pipeline = buildPipeline(*sessionInfo);

        if (not pipeline)
        {
            Logger::error("[CapturePipeline] Pipeline creation failed.");
            throw std::runtime_error("Pipeline creation failed.");
        }

        MediaRaii::GMainLoopPtr mainLoop{
            g_main_loop_new(nullptr, FALSE)
        };

        if (not mainLoop)
        {
            Logger::error("[CapturePipeline] Failed to create GLib main loop.");
            throw std::runtime_error("Failed to create GLib main loop.");
        }

        const GstStateChangeReturn state =
            gst_element_set_state(
                pipeline.get(),
                GST_STATE_PLAYING
            );

        if (state == GST_STATE_CHANGE_FAILURE)
        {
            Logger::error(
                "[CapturePipeline] Failed to set pipeline to PLAYING."
            );
            throw std::runtime_error("Failed to set pipeline to PLAYING.");
        }

        Logger::info("[CapturePipeline] Screen-sharing pipeline started.");

        mediaThread = std::jthread(
            [pipeline = std::move(pipeline), mainLoop = std::move(mainLoop)](std::stop_token stopToken) {
                std::stop_callback stopCallback(
                    stopToken,
                    [&mainLoop] {
                        Logger::info("[CapturePipeline] Stopping main loop in media thread.");
                        g_main_loop_quit(mainLoop.get());
                    }
                );

                Logger::info("[CapturePipeline] Starting main loop in media thread.");
                g_main_loop_run(mainLoop.get());
                Logger::info("[CapturePipeline] Pipeline resources released.");
            }
        );
    }

    ~CapturePipeline() = default;

    CapturePipeline(const CapturePipeline&) = delete;
    CapturePipeline& operator=(const CapturePipeline&) = delete;
    CapturePipeline(CapturePipeline&&) = delete;
    CapturePipeline& operator=(CapturePipeline&&) = delete;

private:
    std::jthread mediaThread;

    MediaRaii::GstPipelineElementPtr buildPipeline(
        const CaptureSessionInfo& sessionInfo
    )
    {
        auto videoEncoder = VideoEncoders::getBestWithProperties();
        if (not videoEncoder)
        {
            Logger::error("[CapturePipeline] No video encoder available");
            return nullptr;
        }

        const std::string pipelineDescription =
            std::format(
                "webrtcsink name=wsink "
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
                "meta=\"meta,name=waycast\" "

                "{} "
                "! queue max-size-buffers=1 max-size-bytes=0 max-size-time=0 leaky=downstream "
                "! videoconvert "
                "! videoscale "
                "! video/x-raw,format=NV12 "
                "! queue max-size-buffers=1 max-size-bytes=0 max-size-time=0 leaky=downstream "
                "! {} "
                "! h264parse config-interval=-1 "
                "! video/x-h264,profile=constrained-baseline,stream-format=avc,alignment=au "
                "! wsink. "

                "{} "
                "! queue "
                "! audioconvert "
                "! audioresample "
                "! opusenc "
                "! audio/x-opus,rate=48000,channels=2 "
                "! wsink.",
                WAYCAST_WEB_DIR,
                sessionInfo.videoSource,
                *videoEncoder,
                sessionInfo.audioSource
            );

        Logger::debug(
            "[CapturePipeline] Pipeline description:\n{}",
            pipelineDescription
        );

        MediaRaii::GErrorPtr error;

        MediaRaii::GstPipelineElementPtr pipeline{
            gst_parse_launch(
                pipelineDescription.c_str(),
                std::out_ptr(error)
            )
        };

        if (error)
        {
            Logger::error(
                "[CapturePipeline] Failed to create pipeline: {}",
                error->message
            );

            return nullptr;
        }

        if (not pipeline)
        {
            Logger::error(
                "[CapturePipeline] gst_parse_launch returned nullptr."
            );

            return nullptr;
        }

        return pipeline;
    }
};
