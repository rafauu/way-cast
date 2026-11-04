module;

#include <gst/gst.h>

export module media.CapturePipeline;

import std;
import logger;
import media.CaptureSession;
import media.PipelineBuilder;
import media.Raii;
import media.VideoEncoder;

export namespace media
{

class CapturePipeline
{
public:
    CapturePipeline()
    {
        logger::info("[CapturePipeline] Initializing Gstreamer.");
        gst_init(nullptr, nullptr);

        auto sessionInfo = CaptureSession::request();
        if (not sessionInfo)
        {
            logger::error("[CapturePipeline] Failed to obtain capture session.");
            throw std::runtime_error("Failed to obtain capture session.");
        }

        auto videoEncoder = VideoEncoder::getBestWithProperties();
        if (not videoEncoder)
        {
            logger::error("[CapturePipeline] No video encoder available");
            throw std::runtime_error("No video encoder available");
        }

        Raii::GstPipelineElementPtr pipeline = PipelineBuilder::build(*sessionInfo, *videoEncoder);

        if (not pipeline)
        {
            logger::error("[CapturePipeline] Pipeline creation failed.");
            throw std::runtime_error("Pipeline creation failed.");
        }

        Raii::GMainLoopPtr mainLoop{ g_main_loop_new(nullptr, FALSE) };

        if (not mainLoop)
        {
            logger::error("[CapturePipeline] Failed to create GLib main loop.");
            throw std::runtime_error("Failed to create GLib main loop.");
        }

        const GstStateChangeReturn state = gst_element_set_state(pipeline.get(), GST_STATE_PLAYING);

        if (state == GST_STATE_CHANGE_FAILURE)
        {
            logger::error("[CapturePipeline] Failed to set pipeline to PLAYING.");
            throw std::runtime_error("Failed to set pipeline to PLAYING.");
        }

        mediaThread = std::jthread(
            [pipeline = std::move(pipeline), mainLoop = std::move(mainLoop)](std::stop_token stopToken) {
                std::stop_callback stopCallback(
                    stopToken,
                    [&mainLoop] {
                        logger::info("[CapturePipeline] Stopping main media loop.");
                        g_main_loop_quit(mainLoop.get());
                    }
                );

                logger::info("[CapturePipeline] Starting main media loop.");
                g_main_loop_run(mainLoop.get());
                logger::info("[CapturePipeline] Releasing media resources.");
            }
        );

        logger::info("[CapturePipeline] Screen-sharing pipeline started.");
    }

    ~CapturePipeline() = default;

    CapturePipeline(const CapturePipeline&) = delete;
    CapturePipeline& operator=(const CapturePipeline&) = delete;
    CapturePipeline(CapturePipeline&&) = delete;
    CapturePipeline& operator=(CapturePipeline&&) = delete;

private:
    std::jthread mediaThread;
};

}
