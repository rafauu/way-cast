module;

#include <gio/gio.h>
#include <gio/gunixfdlist.h>

export module WaylandPortal;

import std;

export class WaylandPortal {
public:
    struct SessionInfo {
        uint32_t pipewire_node_id{0};

        // Preferred identifier for PipeWire stream targeting.
        // This corresponds to PipeWire object.serial.
        uint64_t pipewire_serial{0};

        int pipewire_fd{-1};

        GDBusConnection* dbus_conn{nullptr};
    };

private:
    static constexpr const char* PORTAL_BUS =
        "org.freedesktop.portal.Desktop";

    static constexpr const char* PORTAL_PATH =
        "/org/freedesktop/portal/desktop";

    static constexpr const char* SCREENCAST_IFACE =
        "org.freedesktop.portal.ScreenCast";

    static constexpr const char* REQUEST_IFACE =
        "org.freedesktop.portal.Request";

    struct PortalContext {
        GDBusConnection* conn{nullptr};
        GMainLoop* loop{nullptr};

        std::promise<SessionInfo>* promise{nullptr};

        std::string session_handle;

        uint32_t node_id{0};
        uint64_t pipewire_serial{0};

        int pipewire_fd{-1};

        bool create_session_done{false};
        bool select_sources_done{false};
        bool start_done{false};
        bool promise_completed{false};
    };

    static std::string makeToken(std::string_view prefix) {
        static std::atomic_uint64_t counter{0};

        const auto id =
            counter.fetch_add(1, std::memory_order_relaxed);

        return std::format("{}_{}", prefix, id);
    }

    static void complete(
        PortalContext* ctx,
        SessionInfo info
    ) {
        if (ctx->promise_completed)
            return;

        ctx->promise_completed = true;

        try {
            ctx->promise->set_value(info);
        }
        catch (const std::future_error&) {
            // Promise was already satisfied.
        }

        if (ctx->loop) {
            g_main_loop_quit(ctx->loop);
        }
    }

    static void fail(
        PortalContext* ctx,
        const char* reason
    ) {
        std::cerr
            << "[WaylandPortal] "
            << reason
            << std::endl;

        complete(
            ctx,
            SessionInfo{}
        );
    }

    static bool getUint32Property(
        GVariant* dict,
        const char* key,
        uint32_t& out
    ) {
        GVariant* value =
            g_variant_lookup_value(
                dict,
                key,
                G_VARIANT_TYPE_UINT32
            );

        if (!value)
            return false;

        out = g_variant_get_uint32(value);

        g_variant_unref(value);

        return true;
    }

    static bool getUint64Property(
        GVariant* dict,
        const char* key,
        uint64_t& out
    ) {
        GVariant* value =
            g_variant_lookup_value(
                dict,
                key,
                G_VARIANT_TYPE_UINT64
            );

        if (!value)
            return false;

        out = g_variant_get_uint64(value);

        g_variant_unref(value);

        return true;
    }

    static bool callSelectSources(
        PortalContext* ctx
    ) {
        std::cout
            << "[WaylandPortal] "
               "SelectSources: requesting monitor only..."
            << std::endl;

        GVariantBuilder options;

        g_variant_builder_init(
            &options,
            G_VARIANT_TYPE_VARDICT
        );

        const std::string token =
            makeToken("select");

        /*
         * 1 = MONITOR
         *
         * We deliberately do NOT request WINDOW here.
         * This removes any ambiguity while debugging screen capture.
         */
        g_variant_builder_add(
            &options,
            "{sv}",
            "types",
            g_variant_new_uint32(1)
        );

        /*
         * Only one source.
         */
        g_variant_builder_add(
            &options,
            "{sv}",
            "multiple",
            g_variant_new_boolean(FALSE)
        );

        g_variant_builder_add(
            &options,
            "{sv}",
            "handle_token",
            g_variant_new_string(token.c_str())
        );

        GError* error = nullptr;

        GVariant* params =
            g_variant_new(
                "(oa{sv})",
                ctx->session_handle.c_str(),
                &options
            );

        g_dbus_connection_call(
            ctx->conn,
            PORTAL_BUS,
            PORTAL_PATH,
            SCREENCAST_IFACE,
            "SelectSources",
            params,
            nullptr,
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            nullptr,
            nullptr,
            &error
        );

        if (error) {
            std::cerr
                << "[WaylandPortal] "
                   "SelectSources failed: "
                << error->message
                << std::endl;

            g_error_free(error);

            return false;
        }

        return true;
    }

    static bool callStart(
        PortalContext* ctx
    ) {
        std::cout
            << "[WaylandPortal] "
               "Start: opening source-selection dialog..."
            << std::endl;

        GVariantBuilder options;

        g_variant_builder_init(
            &options,
            G_VARIANT_TYPE_VARDICT
        );

        const std::string token =
            makeToken("start");

        g_variant_builder_add(
            &options,
            "{sv}",
            "handle_token",
            g_variant_new_string(token.c_str())
        );

        /*
         * Empty parent_window is valid for a headless/non-GUI
         * application.
         */
        GVariant* params =
            g_variant_new(
                "(osa{sv})",
                ctx->session_handle.c_str(),
                "",
                &options
            );

        GError* error = nullptr;

        g_dbus_connection_call(
            ctx->conn,
            PORTAL_BUS,
            PORTAL_PATH,
            SCREENCAST_IFACE,
            "Start",
            params,
            nullptr,
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            nullptr,
            nullptr,
            &error
        );

        if (error) {
            std::cerr
                << "[WaylandPortal] "
                   "Start failed: "
                << error->message
                << std::endl;

            g_error_free(error);

            return false;
        }

        return true;
    }

    static bool openPipeWireRemote(
        PortalContext* ctx
    ) {
        std::cout
            << "[WaylandPortal] "
               "Opening PipeWire remote..."
            << std::endl;

        /*
         * OpenPipeWireRemote takes:
         *
         *     (oa{sv})
         *
         * and returns:
         *
         *     (h)
         */
        GError* error = nullptr;

        GUnixFDList* fd_list = nullptr;

        GVariant* result =
            g_dbus_connection_call_with_unix_fd_list_sync(
                ctx->conn,

                PORTAL_BUS,
                PORTAL_PATH,

                SCREENCAST_IFACE,
                "OpenPipeWireRemote",

                g_variant_new(
                    "(oa{sv})",
                    ctx->session_handle.c_str(),
                    nullptr
                ),

                G_VARIANT_TYPE("(h)"),

                G_DBUS_CALL_FLAGS_NONE,

                -1,

                nullptr,

                &fd_list,

                nullptr,

                &error
            );

        if (error) {
            std::cerr
                << "[WaylandPortal] "
                   "OpenPipeWireRemote failed: "
                << error->message
                << std::endl;

            g_error_free(error);

            if (fd_list) {
                g_object_unref(fd_list);
            }

            return false;
        }

        if (!result || !fd_list) {
            std::cerr
                << "[WaylandPortal] "
                   "OpenPipeWireRemote returned no FD."
                << std::endl;

            if (result)
                g_variant_unref(result);

            if (fd_list)
                g_object_unref(fd_list);

            return false;
        }

        gint32 handle = -1;

        g_variant_get(
            result,
            "(h)",
            &handle
        );

        if (handle < 0) {
            std::cerr
                << "[WaylandPortal] "
                   "Invalid PipeWire FD handle."
                << std::endl;

            g_variant_unref(result);
            g_object_unref(fd_list);

            return false;
        }

        GError* fd_error = nullptr;

        const int fd =
            g_unix_fd_list_get(
                fd_list,
                handle,
                &fd_error
            );

        if (fd_error) {
            std::cerr
                << "[WaylandPortal] "
                   "Failed to extract PipeWire FD: "
                << fd_error->message
                << std::endl;

            g_error_free(fd_error);

            g_variant_unref(result);
            g_object_unref(fd_list);

            return false;
        }

        ctx->pipewire_fd = fd;

        std::cout
            << "[WaylandPortal] "
               "PipeWire FD = "
            << ctx->pipewire_fd
            << std::endl;

        g_variant_unref(result);
        g_object_unref(fd_list);

        return true;
    }

    static void onPortalResponse(
        GDBusConnection* conn,
        const gchar* sender_name,
        const gchar* object_path,
        const gchar* interface_name,
        const gchar* signal_name,
        GVariant* params,
        gpointer user_data
    ) {
        auto* ctx =
            static_cast<PortalContext*>(user_data);

        if (!ctx || !params)
            return;

        /*
         * We intentionally don't blindly interpret every Response
         * signal as our own. The callback is subscribed to the
         * portal Request interface, but other portal requests can
         * exist in the same process/session.
         *
         * For this single-threaded state machine, only process
         * responses while we're expecting the corresponding phase.
         */

        guint response_code = 0;
        GVariant* results = nullptr;

        g_variant_get(
            params,
            "(u@a{sv})",
            &response_code,
            &results
        );

        if (response_code != 0) {
            std::cerr
                << "[WaylandPortal] "
                   "Portal request cancelled/failed. "
                   "Response code = "
                << response_code
                << std::endl;

            if (results)
                g_variant_unref(results);

            fail(
                ctx,
                "Portal request was cancelled or failed."
            );

            return;
        }

        /*
         * ---------------------------------------------------------
         * CREATE SESSION
         * ---------------------------------------------------------
         *
         * CreateSession response contains:
         *
         *     session_handle : object path represented as string
         */
        if (!ctx->create_session_done) {
            GVariant* session =
                g_variant_lookup_value(
                    results,
                    "session_handle",
                    G_VARIANT_TYPE_STRING
                );

            if (!session) {
                g_variant_unref(results);

                fail(
                    ctx,
                    "CreateSession succeeded but "
                    "session_handle was missing."
                );

                return;
            }

            const gchar* session_handle =
                g_variant_get_string(
                    session,
                    nullptr
                );

            ctx->session_handle =
                session_handle ? session_handle : "";

            g_variant_unref(session);

            if (ctx->session_handle.empty()) {
                g_variant_unref(results);

                fail(
                    ctx,
                    "CreateSession returned an empty session handle."
                );

                return;
            }

            ctx->create_session_done = true;

            std::cout
                << "[WaylandPortal] "
                   "CreateSession succeeded."
                << std::endl;

            std::cout
                << "[WaylandPortal] "
                   "Session handle: "
                << ctx->session_handle
                << std::endl;

            g_variant_unref(results);

            if (!callSelectSources(ctx)) {
                fail(
                    ctx,
                    "Could not call SelectSources."
                );
            }

            return;
        }

        /*
         * ---------------------------------------------------------
         * SELECT SOURCES
         * ---------------------------------------------------------
         *
         * SelectSources normally has no interesting results.
         */
        if (!ctx->select_sources_done) {
            ctx->select_sources_done = true;

            std::cout
                << "[WaylandPortal] "
                   "SelectSources succeeded."
                << std::endl;

            g_variant_unref(results);

            if (!callStart(ctx)) {
                fail(
                    ctx,
                    "Could not call Start."
                );
            }

            return;
        }

        /*
         * ---------------------------------------------------------
         * START
         * ---------------------------------------------------------
         *
         * Start returns:
         *
         *     streams : a(ua{sv})
         *
         * Each stream is:
         *
         *     (node_id, properties)
         *
         * We specifically extract:
         *
         *     source_type
         *     pipewire-serial
         */
        if (!ctx->start_done) {
            GVariant* streams =
                g_variant_lookup_value(
                    results,
                    "streams",
                    G_VARIANT_TYPE("a(ua{sv})")
                );

            if (!streams) {
                g_variant_unref(results);

                fail(
                    ctx,
                    "Start succeeded but no streams were returned."
                );

                return;
            }

            GVariantIter iter;

            g_variant_iter_init(
                &iter,
                streams
            );

            guint32 node_id = 0;
            GVariant* properties = nullptr;

            bool found_stream = false;

            while (
                g_variant_iter_loop(
                    &iter,
                    "(u@a{sv})",
                    &node_id,
                    &properties
                )
            ) {
                if (!properties)
                    continue;

                uint32_t source_type = 0;
                uint64_t pipewire_serial = 0;

                const bool has_source_type =
                    getUint32Property(
                        properties,
                        "source_type",
                        source_type
                    );

                const bool has_serial =
                    getUint64Property(
                        properties,
                        "pipewire-serial",
                        pipewire_serial
                    );

                std::cout
                    << "[WaylandPortal] "
                       "Selected stream:"
                    << std::endl;

                std::cout
                    << "    node_id          = "
                    << node_id
                    << std::endl;

                if (has_source_type) {
                    const char* source_name =
                        source_type == 1 ? "MONITOR" :
                        source_type == 2 ? "WINDOW" :
                        source_type == 4 ? "VIRTUAL" :
                                           "UNKNOWN";

                    std::cout
                        << "    source_type      = "
                        << source_type
                        << " ("
                        << source_name
                        << ")"
                        << std::endl;
                }
                else {
                    std::cout
                        << "    source_type      = "
                           "<missing>"
                        << std::endl;
                }

                if (has_serial) {
                    std::cout
                        << "    pipewire-serial  = "
                        << pipewire_serial
                        << std::endl;
                }
                else {
                    std::cout
                        << "    pipewire-serial  = "
                           "<missing>"
                        << std::endl;
                }

                /*
                 * We requested exactly one source.
                 *
                 * Therefore the first returned stream is the
                 * stream selected by the user.
                 */
                if (!found_stream) {
                    ctx->node_id = node_id;

                    if (has_serial) {
                        ctx->pipewire_serial =
                            pipewire_serial;
                    }

                    found_stream = true;
                }

                g_variant_unref(properties);
                properties = nullptr;
            }

            g_variant_unref(streams);
            g_variant_unref(results);

            if (!found_stream || ctx->node_id == 0) {
                fail(
                    ctx,
                    "Start returned no valid PipeWire stream."
                );

                return;
            }

            /*
             * Important diagnostic:
             *
             * The portal must report MONITOR here when the user
             * selects a monitor.
             */
            if (ctx->pipewire_serial == 0) {
                std::cout
                    << "[WaylandPortal] "
                       "WARNING: pipewire-serial was not "
                       "provided by this portal."
                    << std::endl;

                std::cout
                    << "[WaylandPortal] "
                       "Falling back to node_id."
                    << std::endl;
            }

            ctx->start_done = true;

            /*
             * Now that Start succeeded and we have the selected
             * stream, obtain the PipeWire remote FD.
             */
            if (!openPipeWireRemote(ctx)) {
                fail(
                    ctx,
                    "Could not open PipeWire remote."
                );

                return;
            }

            complete(
                ctx,
                SessionInfo{
                    .pipewire_node_id =
                        ctx->node_id,

                    .pipewire_serial =
                        ctx->pipewire_serial,

                    .pipewire_fd =
                        ctx->pipewire_fd,

                    .dbus_conn =
                        nullptr
                }
            );

            return;
        }

        /*
         * Anything arriving after the state machine has completed
         * is ignored.
         */
        if (results) {
            g_variant_unref(results);
        }
    }

public:
    static std::optional<SessionInfo>
    requestScreenCapture() {
        GMainLoop* loop =
            g_main_loop_new(
                nullptr,
                FALSE
            );

        if (!loop) {
            std::cerr
                << "[WaylandPortal] "
                   "Failed to create GLib main loop."
                << std::endl;

            return std::nullopt;
        }

        std::promise<SessionInfo> promise;

        auto future =
            promise.get_future();

        GError* connection_error = nullptr;

        GDBusConnection* conn =
            g_bus_get_sync(
                G_BUS_TYPE_SESSION,
                nullptr,
                &connection_error
            );

        if (!conn) {
            if (connection_error) {
                std::cerr
                    << "[WaylandPortal] "
                       "Failed to connect to session D-Bus: "
                    << connection_error->message
                    << std::endl;

                g_error_free(connection_error);
            }
            else {
                std::cerr
                    << "[WaylandPortal] "
                       "Failed to connect to session D-Bus."
                    << std::endl;
            }

            g_main_loop_unref(loop);

            return std::nullopt;
        }

        PortalContext ctx{
            .conn = conn,
            .loop = loop,
            .promise = &promise
        };

        /*
         * Subscribe to Request::Response.
         *
         * The state machine below determines whether the response
         * belongs to CreateSession, SelectSources, or Start.
         */
        const guint subscription_id =
            g_dbus_connection_signal_subscribe(
                conn,

                PORTAL_BUS,

                REQUEST_IFACE,

                "Response",

                nullptr,

                nullptr,

                G_DBUS_SIGNAL_FLAGS_NONE,

                &WaylandPortal::onPortalResponse,

                &ctx,

                nullptr
            );

        /*
         * ---------------------------------------------------------
         * CREATE SESSION
         * ---------------------------------------------------------
         */
        std::cout
            << "[WaylandPortal] "
               "Creating ScreenCast session..."
            << std::endl;

        GVariantBuilder options;

        g_variant_builder_init(
            &options,
            G_VARIANT_TYPE_VARDICT
        );

        const std::string create_token =
            makeToken("create");

        g_variant_builder_add(
            &options,
            "{sv}",
            "handle_token",
            g_variant_new_string(
                create_token.c_str()
            )
        );

        /*
         * CreateSession also accepts session_handle_token.
         *
         * This is useful with portal implementations that support
         * explicitly generated session handles.
         */
        const std::string session_token =
            makeToken("session");

        g_variant_builder_add(
            &options,
            "{sv}",
            "session_handle_token",
            g_variant_new_string(
                session_token.c_str()
            )
        );

        GError* create_error = nullptr;

        g_dbus_connection_call(
            conn,

            PORTAL_BUS,
            PORTAL_PATH,

            SCREENCAST_IFACE,
            "CreateSession",

            g_variant_new(
                "(a{sv})",
                &options
            ),

            nullptr,

            G_DBUS_CALL_FLAGS_NONE,

            -1,

            nullptr,

            nullptr,

            &create_error
        );

        if (create_error) {
            std::cerr
                << "[WaylandPortal] "
                   "CreateSession failed: "
                << create_error->message
                << std::endl;

            g_error_free(create_error);

            g_dbus_connection_signal_unsubscribe(
                conn,
                subscription_id
            );

            g_object_unref(conn);
            g_main_loop_unref(loop);

            return std::nullopt;
        }

        std::cout
            << "[WaylandPortal] "
               "Waiting for monitor selection..."
            << std::endl;

        /*
         * GLib's default main context must run for the portal
         * Response signals to arrive.
         */
        std::jthread glib_thread(
            [loop] {
                g_main_loop_run(loop);
            }
        );

        /*
         * Wait until:
         *
         * CreateSession
         * -> SelectSources
         * -> Start
         * -> OpenPipeWireRemote
         *
         * or until the user cancels/times out.
         */
        const auto status =
            future.wait_for(
                std::chrono::seconds(60)
            );

        if (status == std::future_status::timeout) {
            std::cerr
                << "[WaylandPortal] "
                   "Timed out waiting for portal."
                << std::endl;

            g_main_loop_quit(loop);

            if (glib_thread.joinable())
                glib_thread.join();

            g_dbus_connection_signal_unsubscribe(
                conn,
                subscription_id
            );

            g_object_unref(conn);
            g_main_loop_unref(loop);

            return std::nullopt;
        }

        SessionInfo info =
            future.get();

        /*
         * Stop GLib event loop.
         */
        g_main_loop_quit(loop);

        if (glib_thread.joinable())
            glib_thread.join();

        /*
         * Stop receiving portal responses.
         */
        g_dbus_connection_signal_unsubscribe(
            conn,
            subscription_id
        );

        /*
         * Keep the connection alive in SessionInfo.
         *
         * The caller owns the reference returned by g_bus_get_sync().
         */
        info.dbus_conn = conn;

        g_main_loop_unref(loop);

        if (info.pipewire_fd < 0) {
            std::cerr
                << "[WaylandPortal] "
                   "Invalid PipeWire FD."
                << std::endl;

            g_object_unref(conn);

            return std::nullopt;
        }

        if (info.pipewire_node_id == 0) {
            std::cerr
                << "[WaylandPortal] "
                   "Invalid PipeWire node ID."
                << std::endl;

            g_object_unref(conn);

            return std::nullopt;
        }

        std::cout
            << "[WaylandPortal] "
               "ScreenCast session ready."
            << std::endl;

        std::cout
            << "    node_id         = "
            << info.pipewire_node_id
            << std::endl;

        std::cout
            << "    pipewire_serial = "
            << info.pipewire_serial
            << std::endl;

        std::cout
            << "    pipewire_fd     = "
            << info.pipewire_fd
            << std::endl;

        return info;
    }
};

