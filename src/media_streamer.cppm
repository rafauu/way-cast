module;
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

export module media_streamer;
import std;
import core_queue;

export namespace HlsCast::Media {
    class Streamer {
        Core::FrameQueue& m_queue;
        std::uint32_t m_shared_node_id;
        gint m_pipewire_fd;
        GMainLoop* m_loop = nullptr;
    public:
        explicit Streamer(GMainLoop* loop, Core::FrameQueue& q, std::uint32_t shared_node_id, gint pipewire_fd)
          : m_loop {loop},
            m_queue(q),
            m_shared_node_id {shared_node_id},
            m_pipewire_fd {pipewire_fd}
        {}

        void start(std::stop_token st)
        {
            std::println("Streamer: start");
            gst_init(nullptr, nullptr);

            //auto ps_context = g_main_context_new();
            //g_main_context_push_thread_default(ps_context);
            //m_loop = g_main_loop_new(ps_context, FALSE);
       //     GMainContext* context = g_main_context_new();
   // g_main_context_push_thread_default(context);
            //m_loop = g_main_loop_new(nullptr, FALSE);
            //gst_debug_set_default_threshold(GST_LEVEL_DEBUG);
            //g_log_set_handler(NULL, G_LOG_LEVEL_MASK, g_log_default_handler, NULL);
            //gst_debug_set_colored(TRUE);
    //        std::string pipe =
    //            "pipewiresrc path=" + std::to_string (m_shared_node_id) + " fd=" + std::to_string (m_pipewire_fd) + " ! "
    //            "videoconvert ! "
    //            "video/x-raw,format=I420 ! "
    //            "queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 leaky=downstream ! "
    //            "openh264enc bitrate=8000000 multi-thread=4 usage-type=camera complexity=0 gop-size=15 ! "
    //            "h264parse ! "
    //            "mpegtsmux alignment=0 ! "
    //            "queue max-size-buffers=30 ! "
    //            "appsink name=sink emit-signals=true sync=false async=false";

           // std::string pipe = std::format (
           //     "pipewiresrc fd={} path={} ! videoconvert ! video/x-raw,format=I420 ! appsink name=sink emit-signals=true sync=false",
           //     m_pipewire_fd, m_shared_node_id
           // );

            std::string pipe = std::format (
                "pipewiresrc fd={} path={} do-timestamp=true ! "
                "video/x-raw ! "
                "videoconvert ! "
                "video/x-raw,format=I420 ! "
                "queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 leaky=downstream ! "
                "openh264enc bitrate=8000000 multi-thread=4 usage-type=camera complexity=0 gop-size=15 ! "
                "h264parse ! "
                "mpegtsmux alignment=0 ! "
                "queue max-size-buffers=30 ! "
                "appsink name=sink emit-signals=true sync=false async=false",
                m_pipewire_fd, m_shared_node_id
              );
//
//GError* err = nullptr;
//auto m_pipeline = gst_parse_launch(pipe2.c_str(), &err);
//
//if (!m_pipeline) {
//    if (err) {
//        std::print("GStreamer Error: {}\n", err->message);
//        // If it says "no element 'videotestsrc'", the plugin isn't in that folder
//        g_error_free(err);
//    } else {
//        std::print("Pipeline is null, but no error was reported. Is gst_init called?\n");
//    }
//}

 //           std::string pipe = "videotestsrc ! appsink name=sink emit-signals=true";

            std::println("Streamer: parse launch");
            auto pipeline = gst_parse_launch(pipe.c_str(), nullptr);
            if (not pipeline)
              std::println ("no pipeline");

            std::println("Streamer: get pipeline");
            auto sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
            if (not sink)
              std::println ("no sink");

            g_signal_connect(sink, "new-sample", G_CALLBACK(+[](GstElement* s, gpointer d) {
                auto q = static_cast<Core::FrameQueue*>(d);
                auto sample = gst_app_sink_pull_sample(GST_APP_SINK(s));
                auto buf = gst_sample_get_buffer(sample);
                GstMapInfo map;
                if (gst_buffer_map(buf, &map, GST_MAP_READ))
                {
                    q->enqueue(std::vector<std::uint8_t>(map.data, map.data + map.size));
                    gst_buffer_unmap(buf, &map);
                }
                gst_sample_unref(sample);
                return GST_FLOW_OK;
            }), &m_queue);

            std::println("Streamer: getbus");

            GstBus* bus = gst_element_get_bus(pipeline);
    gst_bus_add_watch(bus, [](GstBus*, GstMessage* msg, gpointer data) -> gboolean {
        auto self = static_cast<Streamer*>(data);
        if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
            GError* err;
            gst_message_parse_error(msg, &err, nullptr);
            std::println("Bus Error: {}", err->message);
            g_main_loop_quit(self->m_loop);
        }
        return TRUE;
    }, this);
    gst_object_unref(bus);
            //std::println("Streamer: set ready");
            //gst_element_set_state(pipeline, GST_STATE_READY);
            std::println("Streamer: set playing");
            gst_element_set_state(pipeline, GST_STATE_PLAYING);

//g_idle_add([](gpointer data) -> gboolean {
//    auto pipe = static_cast<GstElement*>(data);
//    std::println("Idle handler: Triggering PLAYING state from within the loop context...");
//    gst_element_set_state(pipe, GST_STATE_PLAYING);
//    return G_SOURCE_REMOVE; // Run once and stop
//}, pipeline);
//
//            std::println("Streamer: set playing done");

            std::stop_callback stop_cb(st, [this]() {
                if (m_loop && g_main_loop_is_running(m_loop)) {
                    g_main_loop_quit(m_loop);
                }
            });

            std::println("Streamer: GMainLoop starting...");
            g_main_loop_run(m_loop);
            std::println("Streamer: Shutting down...");

            gst_element_set_state(pipeline, GST_STATE_NULL);
            gst_object_unref(sink);
            gst_object_unref(pipeline);
            g_main_loop_unref(m_loop);
            m_loop = nullptr;
        }
    };
}
