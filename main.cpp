#include <iostream>
#include <gio/gio.h>

#include <cstdlib>
#include <csignal>
#include <pthread.h>

import std;
import hls_engine;
import core_queue;
import media_streamer;
import media_processor;
import net_server;



GMainLoop *loop = nullptr;
uint32_t shared_node_id = 0;
char *session_handle = nullptr;

// 1. We need this to store the session between steps
std::string global_session_handle = "";

static void on_portal_response(GDBusConnection *conn, const gchar *sender, const gchar *path,
                               const gchar *iface, const gchar *signal, GVariant *params, gpointer data) {
    uint32_t response_code;
    GVariantIter *results;
    g_variant_get(params, "(ua{sv})", &response_code, &results);

    if (response_code != 0) {
        std::cerr << "Portal Error or Cancelled. Code: " << response_code << std::endl;
        g_main_loop_quit(loop);
        return;
    }

    const gchar *key;
    GVariant *value;
    bool has_session = false;
    bool has_streams = false;

    while (g_variant_iter_loop(results, "{&sv}", &key, &value)) {
        // --- EVENT A: Session created ---
        if (g_strcmp0(key, "session_handle") == 0) {
            global_session_handle = g_variant_get_string(value, NULL);
            has_session = true;
        }
        // --- EVENT B: User accepted & shared streams ---
        if (g_strcmp0(key, "streams") == 0) {
            has_streams = true;
            GVariantIter *stream_iter;
            g_variant_get(value, "a(ua{sv})", &stream_iter);
            if (g_variant_iter_loop(stream_iter, "(ua{sv})", &shared_node_id, NULL)) {
                std::cout << "SUCCESS! PipeWire Node ID: " << shared_node_id << std::endl;
                g_main_loop_quit(loop);
            }
            g_variant_iter_free(stream_iter);
        }
    }

    // --- STATE MACHINE LOGIC ---

    // If we just got a session, we must now call SelectSources
    if (has_session) {
        std::cout << "Step 2: Selecting Sources..." << std::endl;
        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE_VARDICT);
        g_variant_builder_add(&b, "{sv}", "types", g_variant_new_uint32(1 | 2)); // Screen + Window
        g_variant_builder_add(&b, "{sv}", "handle_token", g_variant_new_string("select_token_1"));

        g_dbus_connection_call(conn, "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
            "org.freedesktop.portal.ScreenCast", "SelectSources",
            g_variant_new("(oa{sv})", global_session_handle.c_str(), &b),
            NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
    // If SelectSources finished (but no streams yet), now we call Start
    else if (!has_streams) {
        std::cout << "Step 3: Triggering UI Popup (Start)..." << std::endl;
        GVariantBuilder b;
        g_variant_builder_init(&b, G_VARIANT_TYPE_VARDICT);
        g_variant_builder_add(&b, "{sv}", "handle_token", g_variant_new_string("start_token_1"));

        g_dbus_connection_call(conn, "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
            "org.freedesktop.portal.ScreenCast", "Start",
            g_variant_new("(osa{sv})", global_session_handle.c_str(), "", &b),
            NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);
    }
}

int main() {
    loop = g_main_loop_new(NULL, FALSE);
    GDBusConnection *conn = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);

    // Listen for ALL portal responses
    g_dbus_connection_signal_subscribe(conn, "org.freedesktop.portal.Desktop",
        "org.freedesktop.portal.Request", "Response", NULL, NULL,
        G_DBUS_SIGNAL_FLAGS_NONE, on_portal_response, NULL, NULL);

    // Step 1: Create Session
    GVariantBuilder b;
    g_variant_builder_init(&b, G_VARIANT_TYPE_VARDICT);
    g_variant_builder_add(&b, "{sv}", "handle_token", g_variant_new_string("hls_token"));
    g_variant_builder_add(&b, "{sv}", "session_handle_token", g_variant_new_string("hls_token"));

    g_dbus_connection_call(conn, "org.freedesktop.portal.Desktop", "/org/freedesktop/portal/desktop",
        "org.freedesktop.portal.ScreenCast", "CreateSession",
        g_variant_new("(a{sv})", &b), NULL, G_DBUS_CALL_FLAGS_NONE, -1, NULL, NULL, NULL);

    std::cout << "Waiting for screen selection..." << std::endl;
    g_main_loop_run(loop);

    if (shared_node_id != 0) {
        std::cout << "GStreamer Path ID: " << shared_node_id << std::endl;
        // Now you can safely use: "pipewiresrc path=" + std::to_string(shared_node_id)
    }

    sigset_t sigset;
    sigemptyset (&sigset);
    sigaddset (&sigset, SIGINT);
    pthread_sigmask (SIG_BLOCK, &sigset, nullptr);

    std::println ("Starting Server...");

    using namespace HlsCast;

    Core::FrameQueue queue {2048};
    HlsManager hls;

    Media::Streamer streamer {queue, shared_node_id};
    Media::Processor processor {queue, hls};
    Network::Server server {hls, 8080};

    std::jthread t1 {[&] (std::stop_token st) { streamer.start(st); }};
    std::jthread t2 {[&] (std::stop_token st) { processor.start(st); }};
    std::jthread t3 {[&] (std::stop_token st) { server.run(st); }};

    std::println ("Running...");

    int sig;
    sigwait (&sigset, &sig);
    std::println ("Shutting down via signal {}", sig);


}
