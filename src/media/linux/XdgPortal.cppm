module;

#include <gio/gio.h>
#include <gio/gunixfdlist.h>

export module media.XdgPortal;

import std;
import logger;

export namespace media
{

class XdgPortal
{
public:
    struct SessionInfo
    {
        uint32_t pipewire_node_id{0};
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

    struct PortalContext
    {
        GDBusConnection* conn{nullptr};
        GMainLoop* loop{nullptr};
        std::promise<SessionInfo>* promise{nullptr};
        std::string session_handle{};
        uint32_t node_id{0};
        uint64_t pipewire_serial{0};
        int pipewire_fd{-1};
        bool create_session_done{false};
        bool select_sources_done{false};
        bool start_done{false};
        bool promise_completed{false};
    };

    static std::string makeToken(std::string_view prefix)
    {
        static std::atomic_uint64_t counter{0};

        const auto id =
            counter.fetch_add(1, std::memory_order_relaxed);

        return std::format("{}_{}", prefix, id);
    }

    static void complete(PortalContext* ctx, SessionInfo info)
    {
        if (ctx->promise_completed)
        {
            return;
        }

        ctx->promise_completed = true;

        try
        {
            ctx->promise->set_value(info);
        }
        catch (const std::future_error&)
        {
            // Promise was already satisfied.
        }

        if (ctx->loop)
        {
            g_main_loop_quit(ctx->loop);
        }
    }

    static void fail(PortalContext* ctx, const char* reason)
    {
        logger::error("[XdgPortal] {}", reason);
        complete(ctx, SessionInfo{});
    }

    static bool getUint32Property(
        GVariant* dict,
        const char* key,
        uint32_t& out)
    {
        GVariant* value = g_variant_lookup_value(
            dict, key, G_VARIANT_TYPE_UINT32);

        if (!value)
        {
            return false;
        }

        out = g_variant_get_uint32(value);
        g_variant_unref(value);
        return true;
    }

    static bool getUint64Property(
        GVariant* dict,
        const char* key,
        uint64_t& out)
    {
        GVariant* value = g_variant_lookup_value(
            dict, key, G_VARIANT_TYPE_UINT64);

        if (!value)
        {
            return false;
        }

        out = g_variant_get_uint64(value);
        g_variant_unref(value);
        return true;
    }

    static bool callSelectSources(PortalContext* ctx)
    {
        logger::info(
            "[XdgPortal] SelectSources: requesting monitor only...");

        GVariantBuilder options;
        g_variant_builder_init(&options, G_VARIANT_TYPE_VARDICT);

        const std::string token = makeToken("select");

        // 1 = MONITOR. Only monitor capture is requested.
        g_variant_builder_add(
            &options, "{sv}", "types", g_variant_new_uint32(1));

        g_variant_builder_add(
            &options, "{sv}", "multiple", g_variant_new_boolean(FALSE));

        g_variant_builder_add(
            &options, "{sv}", "handle_token",
            g_variant_new_string(token.c_str()));

        GError* error = nullptr;

        GVariant* params = g_variant_new(
            "(oa{sv})", ctx->session_handle.c_str(), &options);

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
            &error);

        if (error)
        {
            logger::error(
                "[XdgPortal] SelectSources failed: {}",
                error->message);
            g_error_free(error);
            return false;
        }

        return true;
    }

    static bool callStart(PortalContext* ctx)
    {
        logger::info(
            "[XdgPortal] Start: opening source-selection dialog...");

        GVariantBuilder options;
        g_variant_builder_init(&options, G_VARIANT_TYPE_VARDICT);

        const std::string token = makeToken("start");

        g_variant_builder_add(
            &options, "{sv}", "handle_token",
            g_variant_new_string(token.c_str()));

        GVariant* params = g_variant_new(
            "(osa{sv})", ctx->session_handle.c_str(), "", &options);

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
            &error);

        if (error)
        {
            logger::error(
                "[XdgPortal] Start failed: {}",
                error->message);
            g_error_free(error);
            return false;
        }

        return true;
    }

    static bool openPipeWireRemote(PortalContext* ctx)
    {
        logger::info("[XdgPortal] Opening PipeWire remote...");

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
                    "(oa{sv})", ctx->session_handle.c_str(), nullptr),
                G_VARIANT_TYPE("(h)"),
                G_DBUS_CALL_FLAGS_NONE,
                -1,
                nullptr,
                &fd_list,
                nullptr,
                &error);

        if (error)
        {
            logger::error(
                "[XdgPortal] OpenPipeWireRemote failed: {}",
                error->message);
            g_error_free(error);

            if (fd_list)
            {
                g_object_unref(fd_list);
            }

            return false;
        }

        if (!result || !fd_list)
        {
            logger::error(
                "[XdgPortal] OpenPipeWireRemote returned no FD.");

            if (result)
            {
                g_variant_unref(result);
            }

            if (fd_list)
            {
                g_object_unref(fd_list);
            }

            return false;
        }

        gint32 handle = -1;
        g_variant_get(result, "(h)", &handle);

        if (handle < 0)
        {
            logger::error("[XdgPortal] Invalid PipeWire FD handle.");
            g_variant_unref(result);
            g_object_unref(fd_list);
            return false;
        }

        GError* fd_error = nullptr;

        const int fd = g_unix_fd_list_get(
            fd_list, handle, &fd_error);

        if (fd_error)
        {
            logger::error(
                "[XdgPortal] Failed to extract PipeWire FD: {}",
                fd_error->message);
            g_error_free(fd_error);
            g_variant_unref(result);
            g_object_unref(fd_list);
            return false;
        }

        ctx->pipewire_fd = fd;

        logger::info(
            "[XdgPortal] PipeWire FD = {}", ctx->pipewire_fd);

        g_variant_unref(result);
        g_object_unref(fd_list);
        return true;
    }

    static void onPortalResponse(
        GDBusConnection*,
        const gchar*,
        const gchar*,
        const gchar*,
        const gchar*,
        GVariant* params,
        gpointer user_data)
    {
        auto* ctx = static_cast<PortalContext*>(user_data);

        if (!ctx || !params)
        {
            return;
        }

        guint response_code = 0;
        GVariant* results = nullptr;

        g_variant_get(params, "(u@a{sv})", &response_code, &results);

        if (response_code != 0)
        {
            logger::error(
                "[XdgPortal] Portal request cancelled/failed. "
                "Response code = {}",
                response_code);

            if (results)
            {
                g_variant_unref(results);
            }

            fail(ctx, "Portal request was cancelled or failed.");
            return;
        }

        // CREATE SESSION
        if (!ctx->create_session_done)
        {
            GVariant* session = g_variant_lookup_value(
                results, "session_handle", G_VARIANT_TYPE_STRING);

            if (!session)
            {
                g_variant_unref(results);
                fail(ctx,
                    "CreateSession succeeded but session_handle was missing.");
                return;
            }

            const gchar* session_handle =
                g_variant_get_string(session, nullptr);

            ctx->session_handle =
                session_handle ? session_handle : "";

            g_variant_unref(session);

            if (ctx->session_handle.empty())
            {
                g_variant_unref(results);
                fail(ctx, "CreateSession returned an empty session handle.");
                return;
            }

            ctx->create_session_done = true;

            logger::info("[XdgPortal] CreateSession succeeded.");
            logger::debug(
                "[XdgPortal] Session handle: {}",
                ctx->session_handle);

            g_variant_unref(results);

            if (!callSelectSources(ctx))
            {
                fail(ctx, "Could not call SelectSources.");
            }

            return;
        }

        // SELECT SOURCES
        if (!ctx->select_sources_done)
        {
            ctx->select_sources_done = true;

            logger::info("[XdgPortal] SelectSources succeeded.");

            g_variant_unref(results);

            if (!callStart(ctx))
            {
                fail(ctx, "Could not call Start.");
            }

            return;
        }

        // START
        if (!ctx->start_done)
        {
            GVariant* streams = g_variant_lookup_value(
                results, "streams", G_VARIANT_TYPE("a(ua{sv})"));

            if (!streams)
            {
                g_variant_unref(results);
                fail(ctx, "Start succeeded but no streams were returned.");
                return;
            }

            GVariantIter iter;
            g_variant_iter_init(&iter, streams);

            guint32 node_id = 0;
            GVariant* properties = nullptr;
            bool found_stream = false;

            while (g_variant_iter_loop(
                &iter, "(u@a{sv})", &node_id, &properties))
            {
                if (!properties)
                {
                    continue;
                }

                uint32_t source_type = 0;
                uint64_t pipewire_serial = 0;

                const bool has_source_type = getUint32Property(
                    properties, "source_type", source_type);

                const bool has_serial = getUint64Property(
                    properties, "pipewire-serial", pipewire_serial);

                logger::info(
                    "[XdgPortal] Selected stream: node_id={}",
                    node_id);

                if (has_source_type)
                {
                    const char* source_name =
                        source_type == 1 ? "MONITOR" :
                        source_type == 2 ? "WINDOW" :
                        source_type == 4 ? "VIRTUAL" :
                                           "UNKNOWN";

                    logger::info(
                        "[XdgPortal] source_type={} ({})",
                        source_type, source_name);
                }
                else
                {
                    logger::warning(
                        "[XdgPortal] source_type=<missing>");
                }

                if (has_serial)
                {
                    logger::info(
                        "[XdgPortal] pipewire-serial={}",
                        pipewire_serial);
                }
                else
                {
                    logger::warning(
                        "[XdgPortal] pipewire-serial=<missing>");
                }

                if (!found_stream)
                {
                    ctx->node_id = node_id;

                    if (has_serial)
                    {
                        ctx->pipewire_serial = pipewire_serial;
                    }

                    found_stream = true;
                }

                g_variant_unref(properties);
                properties = nullptr;
            }

            g_variant_unref(streams);
            g_variant_unref(results);

            if (!found_stream || ctx->node_id == 0)
            {
                fail(ctx, "Start returned no valid PipeWire stream.");
                return;
            }

            if (ctx->pipewire_serial == 0)
            {
                logger::warning(
                    "[XdgPortal] pipewire-serial was not provided "
                    "by this portal.");
                logger::warning(
                    "[XdgPortal] Falling back to node_id.");
            }

            ctx->start_done = true;

            if (!openPipeWireRemote(ctx))
            {
                fail(ctx, "Could not open PipeWire remote.");
                return;
            }

            complete(
                ctx,
                SessionInfo{
                    .pipewire_node_id = ctx->node_id,
                    .pipewire_serial = ctx->pipewire_serial,
                    .pipewire_fd = ctx->pipewire_fd,
                    .dbus_conn = nullptr
                });

            return;
        }

        if (results)
        {
            g_variant_unref(results);
        }
    }

public:
    static std::optional<SessionInfo> requestScreenCapture()
    {
        GMainLoop* loop = g_main_loop_new(nullptr, FALSE);

        if (!loop)
        {
            logger::error(
                "[XdgPortal] Failed to create GLib main loop.");
            return std::nullopt;
        }

        std::promise<SessionInfo> promise;
        auto future = promise.get_future();

        GError* connection_error = nullptr;

        GDBusConnection* conn = g_bus_get_sync(
            G_BUS_TYPE_SESSION, nullptr, &connection_error);

        if (!conn)
        {
            if (connection_error)
            {
                logger::error(
                    "[XdgPortal] Failed to connect to session D-Bus: {}",
                    connection_error->message);
                g_error_free(connection_error);
            }
            else
            {
                logger::error(
                    "[XdgPortal] Failed to connect to session D-Bus.");
            }

            g_main_loop_unref(loop);
            return std::nullopt;
        }

        PortalContext ctx{
            .conn = conn,
            .loop = loop,
            .promise = &promise
        };

        const guint subscription_id =
            g_dbus_connection_signal_subscribe(
                conn,
                PORTAL_BUS,
                REQUEST_IFACE,
                "Response",
                nullptr,
                nullptr,
                G_DBUS_SIGNAL_FLAGS_NONE,
                &XdgPortal::onPortalResponse,
                &ctx,
                nullptr);

        // CREATE SESSION
        logger::info(
            "[XdgPortal] Creating ScreenCast session...");

        GVariantBuilder options;
        g_variant_builder_init(&options, G_VARIANT_TYPE_VARDICT);

        const std::string create_token = makeToken("create");

        g_variant_builder_add(
            &options, "{sv}", "handle_token",
            g_variant_new_string(create_token.c_str()));

        const std::string session_token = makeToken("session");

        g_variant_builder_add(
            &options, "{sv}", "session_handle_token",
            g_variant_new_string(session_token.c_str()));

        GError* create_error = nullptr;

        g_dbus_connection_call(
            conn,
            PORTAL_BUS,
            PORTAL_PATH,
            SCREENCAST_IFACE,
            "CreateSession",
            g_variant_new("(a{sv})", &options),
            nullptr,
            G_DBUS_CALL_FLAGS_NONE,
            -1,
            nullptr,
            nullptr,
            &create_error);

        if (create_error)
        {
            logger::error(
                "[XdgPortal] CreateSession failed: {}",
                create_error->message);

            g_error_free(create_error);

            g_dbus_connection_signal_unsubscribe(
                conn, subscription_id);

            g_object_unref(conn);
            g_main_loop_unref(loop);
            return std::nullopt;
        }

        logger::info(
            "[XdgPortal] Waiting for monitor selection...");

        std::jthread glib_thread([loop]
        {
            g_main_loop_run(loop);
        });

        const auto status =
            future.wait_for(std::chrono::seconds(60));

        if (status == std::future_status::timeout)
        {
            logger::error(
                "[XdgPortal] Timed out waiting for portal.");

            g_main_loop_quit(loop);

            if (glib_thread.joinable())
            {
                glib_thread.join();
            }

            g_dbus_connection_signal_unsubscribe(
                conn, subscription_id);

            g_object_unref(conn);
            g_main_loop_unref(loop);
            return std::nullopt;
        }

        SessionInfo info = future.get();

        g_main_loop_quit(loop);

        if (glib_thread.joinable())
        {
            glib_thread.join();
        }

        g_dbus_connection_signal_unsubscribe(
            conn, subscription_id);

        info.dbus_conn = conn;

        g_main_loop_unref(loop);

        if (info.pipewire_fd < 0)
        {
            logger::error("[XdgPortal] Invalid PipeWire FD.");
            g_object_unref(conn);
            return std::nullopt;
        }

        if (info.pipewire_node_id == 0)
        {
            logger::error("[XdgPortal] Invalid PipeWire node ID.");
            g_object_unref(conn);
            return std::nullopt;
        }

        logger::info("[XdgPortal] ScreenCast session ready.");
        logger::info("    node_id         = {}", info.pipewire_node_id);
        logger::info("    pipewire_serial = {}", info.pipewire_serial);
        logger::info("    pipewire_fd     = {}", info.pipewire_fd);

        return info;
    }
};

}
