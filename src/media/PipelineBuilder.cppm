module;

#include <gst/gst.h>

export module media.PipelineBuilder;

import std;
import logger;
import media.core.raii;
import media.CaptureSession;

export namespace media
{

class PipelineBuilder
{
public:
    PipelineBuilder() = delete;

    [[nodiscard]] static core::raii::GstPipelineElementPtr build(
        const CaptureSession::Info& sessionInfo,
        std::string_view videoEncoder
    )
    {
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
                videoEncoder,
                sessionInfo.audioSource
            );

        logger::debug("[PipelineBuilder] Pipeline description:\n{}", pipelineDescription);

        core::raii::GErrorPtr error;

        core::raii::GstPipelineElementPtr pipeline{
            gst_parse_launch(
                pipelineDescription.c_str(),
                std::out_ptr(error)
            )
        };

        if (error)
        {
            logger::error("[PipelineBuilder] Failed to create pipeline: {}", error->message);
            return nullptr;
        }

        if (not pipeline)
        {
            logger::error("[PipelineBuilder] gst_parse_launch returned nullptr.");
            return nullptr;
        }

        return pipeline;
    }
};

}
