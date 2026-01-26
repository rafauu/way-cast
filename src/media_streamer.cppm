module;
#include <gst/gst.h>
#include <gst/app/gstappsink.h>

#include <gio/gio.h>
#include <gst/gst.h>
#include <iostream>

export module media_streamer;
import std;
import core_queue;

/*
// Global variables for tracking
GMainLoop *loop = nullptr;
uint32_t shared_node_id = 0;
std::string session_handle = "";
int portal_step = 0; // 0: CreateSession, 1: SelectSources, 2: Start

// Helper to build the mandatory options with unique tokens
GVariant* get_options_with_token(const std::string& prefix) {
    GVariantBuilder builder;
    g_variant_builder_init(&builder, G_VARIANT_TYPE_VARDICT);
    std::string token = prefix + "_" + std::to_string(rand() % 10000);
    g_variant_builder_add(&builder, "{sv}", "handle_token", g_variant_new_string(token.c_str()));
    if (prefix == "session") {
        g_variant_builder_add(&builder, "{sv}", "session_handle_token", g_variant_new_string(token.c_str()));
    }
    return g_variant_builder_end(&builder);
}

static void on_portal_response(GDBusConnection *conn, const gchar *sender, const gchar *path,
                               const gchar *iface, const gchar *signal, GVariant *params, gpointer data) {
    uint32_t response_code;
    GVariantIter *results;
    g_variant_get(params, "(ua{sv})", &response_code, &results);

    if (response_code != 0) {
        std::cerr << "Portal request failed or cancelled by user." << std::endl;
        g_main_loop_quit(loop);
        return;
    }

    if (portal_step == 0) { // CreateSession finished
        const gchar *key;
        GVariant *value;
        while (g_variant_iter_loop(results, "{&sv}", &key, &value)) {
            if (g_strcmp0(key, "session_handle") == 0) {
                session_handle = g_variant_get_string(value, NULL);
                std::cout << "Step 1: Session Created -> " << session_handle << std::endl;
            }
        }

        // Advance to Step 2: SelectSources
        portal_step = 1;
        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE_VARDICT);
        g_variant_builder_add(&b, "{sv}", "types", g_variant_new_uint32(1)); // 1 = Screen

        g_dbus_connection_call(conn, "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
            "org.freedesktop.portal.ScreenCast", "SelectSources",
            g_variant_new("(oa{sv})", session_handle.c_str(), &b),
            NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);

    } else if (portal_step == 1) { // SelectSources finished
        std::cout << "Step 2: Sources Selected." << std::endl;

        // Advance to Step 3: Start (Triggering the Popup)
        portal_step = 2;
        g_dbus_connection_call(conn, "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
            "org.freedesktop.portal.ScreenCast", "Start",
            g_variant_new("(osa{sv})", session_handle.c_str(), "", get_options_with_token("start")),
            NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);

    } else if (portal_step == 2) { // Start finished (This contains the ID!)
        const gchar *key;
        GVariant *value;
        while (g_variant_iter_loop(results, "{&sv}", &key, &value)) {
            if (g_strcmp0(key, "streams") == 0) {
                GVariantIter *stream_iter;
                g_variant_get(value, "a(ua{sv})", &stream_iter);
                if (g_variant_iter_loop(stream_iter, "(ua{sv})", &shared_node_id, NULL)) {
                    std::cout << "Step 3: Permission Granted! Node ID: " << shared_node_id << std::endl;
                    g_main_loop_quit(loop);
                }
                g_variant_iter_free(stream_iter);
            }
        }
    }
}

void trigger_portal() {
    GError *error = NULL;
    GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, &error);

    // Subscribe to ALL portal responses globally for this connection
    g_dbus_connection_signal_subscribe(conn, "org.freedesktop.portal.Desktop",
        "org.freedesktop.portal.Request", "Response", NULL, NULL,
        G_DBUS_SIGNAL_FLAGS_NONE, on_portal_response, NULL, NULL);

    // Initial Call: CreateSession
    portal_step = 0;
    g_dbus_connection_call(conn, "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.ScreenCast", "CreateSession",
        g_variant_new("(a{sv})", get_options_with_token("session")),
        NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
}
*/

export namespace HlsCast::Media {
    class Streamer {
        Core::FrameQueue& m_queue;
        std::uint32_t m_shared_node_id;
        GMainLoop* m_loop = nullptr;
    public:
        explicit Streamer(Core::FrameQueue& q, std::uint32_t shared_node_id)
          : m_queue(q),
            m_shared_node_id {shared_node_id}
        {}

        void start(std::stop_token st)
        {
            gst_init(nullptr, nullptr);
            m_loop = g_main_loop_new(nullptr, FALSE); //moved here
            // Pipeline: Screen Capture -> HW Encode -> MPEG-TS -> AppSink
            //std::string pipe = "ximagesrc ! videoconvert ! nvh264enc bitrate=5000 gop-size=60 ! "
            //std::string pipe = "autovideosink ! videoconvert ! nvh264enc bitrate=5000 gop-size=60 ! "
            //std::string pipe = "videotestsrc is-live=true ! videoconvert ! openh264enc gop-size=30 usage-type=1 rate-control=bitrate bitrate=500000 ! h264parse ! mpegtsmux ! appsink name=sink emit-signals=true sync=false";
            //trigger_portal();
            //g_main_loop_run(loop);
            //std::cout << std::to_string(shared_node_id);
            std::string pipe =
    "pipewiresrc path=" + std::to_string(m_shared_node_id) + " ! "
    "video/x-raw ! "
    //"videotestsrc is-live=true ! "
    //"video/x-raw,width=1024,height=768,framerate=30/1,format=I420 !"
    //"ximagesrc ! "
 //   "timeoverlay valignment=bottom halignment=right font-desc='Sans, 24' ! "
    "videoconvert ! "
    "video/x-raw,format=I420 ! "
    "queue max-size-buffers=30 max-size-bytes=0 max-size-time=0 leaky=downstream ! "
    //"openh264enc ! "
    "openh264enc bitrate=8000000 multi-thread=4 usage-type=camera complexity=0 gop-size=15 ! "
    "h264parse ! "
    "mpegtsmux alignment=0 ! "
    "queue max-size-buffers=30 ! "
    "appsink name=sink emit-signals=true sync=false async=false";
//            std::string pipe2 = "fakesrc ! fakesink";
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

            auto pipeline = gst_parse_launch(pipe.c_str(), nullptr);
            if (not pipeline)
              std::println ("no pipeline");
            auto sink = gst_bin_get_by_name(GST_BIN(pipeline), "sink");
            if (not sink)
              std::println ("no sink");

            g_signal_connect(sink, "new-sample", G_CALLBACK(+[](GstElement* s, gpointer d) {
                auto q = static_cast<Core::FrameQueue*>(d);
                auto sample = gst_app_sink_pull_sample(GST_APP_SINK(s));
                auto buf = gst_sample_get_buffer(sample);
                GstMapInfo map;
                if (gst_buffer_map(buf, &map, GST_MAP_READ)) {
                    q->enqueue(std::vector<std::uint8_t>(map.data, map.data + map.size));
                    gst_buffer_unmap(buf, &map);
                }
                gst_sample_unref(sample);
                return GST_FLOW_OK;
            }), &m_queue);

            m_loop = g_main_loop_new(nullptr, FALSE);

            gst_element_set_state(pipeline, GST_STATE_PLAYING);

            GstState state;
gst_element_get_state(pipeline, &state, nullptr, GST_SECOND);
if (state != GST_STATE_PLAYING) {
    std::println("Error: Pipeline failed to reach PLAYING state.");

    // Check the bus for the real error message
    GstBus* bus = gst_element_get_bus(pipeline);
    GstMessage* msg = gst_bus_pop_filtered(bus, GST_MESSAGE_ERROR);
    if (msg) {
        GError* err;
        gchar* debug;
        gst_message_parse_error(msg, &err, &debug);
        std::println("Bus Error: {}", err->message);
        g_error_free(err);
        g_free(debug);
        gst_message_unref(msg);
    }
    gst_object_unref(bus);
}

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
