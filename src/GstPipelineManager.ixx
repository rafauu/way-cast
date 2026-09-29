module;

#include <gst/gst.h>
#include <gst/webrtc/webrtc.h>

export module GstPipelineManager;

import MpscQueue;
import WaylandPortal;
import std;

#ifndef VCPKG_GST_PLUGIN_DIR
#define VCPKG_GST_PLUGIN_DIR ""
#endif

static gboolean bus_callback(GstBus* bus, GstMessage* msg, gpointer data) {
    switch (GST_MESSAGE_TYPE(msg)) {
        case GST_MESSAGE_ERROR: {
            GError* err = nullptr;
            gchar* debug_info = nullptr;
            gst_message_parse_error(msg, &err, &debug_info);
            std::println("[GStreamer ERROR] Source: {} | Message: {}",
                GST_OBJECT_NAME(msg->src), err->message);
            if (debug_info) std::println("[GStreamer Debug]: {}", debug_info);
            g_error_free(err);
            g_free(debug_info);
            break;
        }
        case GST_MESSAGE_WARNING: {
            GError* err = nullptr;
            gchar* debug_info = nullptr;
            gst_message_parse_warning(msg, &err, &debug_info);
            std::println("[GStreamer WARNING] Source: {} | Message: {}",
                GST_OBJECT_NAME(msg->src), err->message);
            g_error_free(err);
            g_free(debug_info);
            break;
        }
        case GST_MESSAGE_STATE_CHANGED: {
            if (GST_IS_PIPELINE(msg->src)) {
                GstState old_state, new_state, pending_state;
                gst_message_parse_state_changed(msg, &old_state, &new_state, &pending_state);
                std::println("[GStreamer Pipeline State] Changed from {} to {}",
                    gst_element_state_get_name(old_state),
                    gst_element_state_get_name(new_state));
            }
            break;
        }
        default:
            break;
    }
    return TRUE;
}

std::string escapeJson(const std::string& input) {
        std::string output;
        for (char c : input) {
            switch (c) {
                case '\n': output += "\\n"; break;
                case '\r': output += "\\r"; break;
                case '\"': output += "\\\""; break;
                case '\\': output += "\\\\"; break;
                default:
                    if (static_cast<unsigned char>(c) < 32) {
                        output += std::format("\\u{0:04x}", static_cast<unsigned char>(c));
                    } else {
                        output += c;
                    }
            }
        }
        return output;
    }

export struct InboundEvent {
    enum class Type { RemoteAnswer, RemoteCandidate } type;
    std::size_t session_id{0};
    std::string sdp_or_candidate;
    int mline_index{0};
};

export struct OutboundMessage {
    std::size_t target_session_id{0};
    std::string json_payload;
};

export class GstPipelineManager {
public:
    using InboundQueue  = LockFreeMpscQueue<InboundEvent>;
    using OutboundQueue = LockFreeMpscQueue<OutboundMessage>;

    explicit GstPipelineManager(OutboundQueue& outbound_queue)
        : outbound_queue_(outbound_queue) {}

    ~GstPipelineManager() {
        stop();
    }

    void start() {
        if (std::getenv("GST_PLUGIN_PATH") == nullptr &&
            std::string_view(VCPKG_GST_PLUGIN_DIR).size() > 0) {
            ::setenv("GST_PLUGIN_PATH", VCPKG_GST_PLUGIN_DIR, 1);
        }

        gst_init(nullptr, nullptr);

        running_ = true;
        media_thread_ = std::thread([this] {
            mediaLoop();
        });
    }

    void stop() {
        if (!running_.exchange(false))
            return;

        if (pipeline_) {
            gst_element_set_state(
                pipeline_,
                GST_STATE_NULL
            );
        }

        if (media_thread_.joinable())
            media_thread_.join();

        if (pipeline_) {
            gst_object_unref(pipeline_);
            pipeline_ = nullptr;
            webrtc_ = nullptr;
        }
    }

    void postInbound(InboundEvent evt) {
        inbound_queue_.push(std::move(evt));
    }

    std::optional<OutboundMessage> popOutbound() {
        return outbound_queue_.pop();
    }

    void setActiveSession(std::size_t id) {
        active_session_id_ = id;
        if (!cached_offer_json_.empty()) {
            outbound_queue_.push(OutboundMessage{
                .target_session_id = id,
                .json_payload = cached_offer_json_
            });
            std::println("[WebRTC] Client connected. Pushed cached SDP Offer to session #{}", id);
        }
    }

private:
    InboundQueue inbound_queue_;
    OutboundQueue& outbound_queue_;

    std::string cached_offer_json_;
    std::atomic<std::size_t> active_session_id_{0};
    std::atomic<bool> running_{false};
    std::atomic<bool> offer_generated_{false};

    GstElement* pipeline_{nullptr};
    GstElement* webrtc_{nullptr};

    std::thread media_thread_;

    void mediaLoop() {
        buildPipeline();

        GMainLoop* main_loop = g_main_loop_new(nullptr, FALSE);
        std::jthread glib_thread([main_loop](std::stop_token stoken) {
            std::stop_callback stop_cb(stoken, [main_loop] {
                g_main_loop_quit(main_loop);
            });

            g_main_loop_run(main_loop);
        });

        while (running_) {
            while (auto evt = inbound_queue_.pop()) {
                handleInboundEvent(std::move(*evt));
            }

            std::this_thread::sleep_for(
                std::chrono::milliseconds(2)
            );
        }

        glib_thread.request_stop();

        g_main_loop_unref(main_loop);
    }

    void buildPipeline() {
        auto portal_session =
            WaylandPortal::requestScreenCapture();

        if (not portal_session)
        {
            std::println(
                std::cerr,
                "[GStreamer] Failed to get PipeWire screen capture session."
            );

            return;
        }

        const auto& session = *portal_session;

        std::string targetObject = session.pipewire_serial != 0
                                   ? std::to_string(session.pipewire_serial)
                                   : std::to_string(session.pipewire_node_id);

        const std::string pipe_str = std::format(
            "pipewiresrc fd={} path=\"{}\" do-timestamp=TRUE ! "
            "queue max-size-buffers=2 leaky=downstream ! "
            "videoconvert ! "
            "videoscale ! "
            "videorate ! "
            "video/x-raw,format=I420,width=1920,height=1080,framerate=30/1 ! "
            "queue max-size-buffers=2 leaky=downstream ! "
            "vp8enc cpu-used=8 deadline=1 target-bitrate=2500000 keyframe-max-dist=60 ! "
            "queue max-size-buffers=2 leaky=downstream ! "
            "rtpvp8pay name=vp8pay pt=96",
            portal_session->pipewire_fd,
            targetObject
        );

        std::println(
            "[GStreamer] Creating media pipeline: {}",
            pipe_str
        );

        GError* error = nullptr;

        pipeline_ = gst_parse_launch(pipe_str.c_str(), &error);

        GstBus* bus = gst_element_get_bus(pipeline_);
        gst_bus_add_watch(bus, bus_callback, nullptr);
        gst_object_unref(bus);

        if (error) {
            std::println(
                std::cerr,
                "[GStreamer] Failed to build media pipeline: {}",
                error->message
            );

            g_error_free(error);

            if (pipeline_) {
                gst_object_unref(pipeline_);
                pipeline_ = nullptr;
            }

            return;
        }

        if (!pipeline_) {
            std::println(
                std::cerr,
                "[GStreamer] gst_parse_launch returned null."
            );

            return;
        }

        /*
         * Create webrtcbin separately.
         */
        webrtc_ =
            gst_element_factory_make(
                "webrtcbin",
                "sendrecv"
            );

        if (!webrtc_) {
            std::println(
                std::cerr,
                "[GStreamer] Failed to create webrtcbin."
            );

            gst_object_unref(pipeline_);
            pipeline_ = nullptr;
            return;
        }

        /*
         * Add webrtcbin to the existing pipeline.
         */
        gst_bin_add(
            GST_BIN(pipeline_),
            webrtc_
        );

        /*
         * Find the VP8 payloader.
         */
        GstElement* pay =
            gst_bin_get_by_name(
                GST_BIN(pipeline_),
                "vp8pay"
            );

        if (!pay) {
            std::println(
                std::cerr,
                "[GStreamer] Could not find vp8pay."
            );
            return;
        }

        /*
         * Request WebRTC RTP sink pad via transceiver.
         */
        GstWebRTCRTPTransceiver* transceiver = nullptr;
        g_signal_emit_by_name(
            webrtc_,
            "add-transceiver",
            2, // GST_WEBRTC_RTP_TRANSCEIVER_DIRECTION_SENDONLY
            nullptr,
            &transceiver
        );

        if (!transceiver) {
            std::println(
                std::cerr,
                "[GStreamer] Failed to add transceiver to webrtcbin."
            );
            gst_object_unref(pay);
            return;
        }

        g_object_unref(transceiver);

        if (!gst_element_link_pads(pay, "src", webrtc_, "sink_%u")) {
            std::println(std::cerr, "[GStreamer Error] Failed to link rtpvp8pay to webrtcbin!");
        } else {
            std::println("[GStreamer] Successfully linked video pipeline to webrtcbin!");
        }

        gst_object_unref(pay);

        g_signal_connect(
            webrtc_,
            "on-negotiation-needed",
            G_CALLBACK(onNegotiationNeededStatic),
            this
        );

        g_signal_connect(
            webrtc_,
            "on-ice-candidate",
            G_CALLBACK(onIceCandidateStatic),
            this
        );

        /*
         * Start the complete pipeline.
         */
        const GstStateChangeReturn state =
            gst_element_set_state(
                pipeline_,
                GST_STATE_PLAYING
            );

        if (state == GST_STATE_CHANGE_FAILURE) {
            std::println(
                std::cerr,
                "[GStreamer] Failed to set pipeline to PLAYING."
            );

            if (GstBus* bus = gst_element_get_bus(pipeline_)) {
                if (GstMessage* msg = gst_bus_timed_pop_filtered(bus, 0, GST_MESSAGE_ERROR)) {
                    GError* err = nullptr;
                    gchar* debug_info = nullptr;
                    gst_message_parse_error(msg, &err, &debug_info);

                    std::println(std::cerr, "[GStreamer Error] {}", err->message);
                    if (debug_info) {
                        std::println(std::cerr, "[GStreamer Debug] {}", debug_info);
                        g_free(debug_info);
                    }

                    g_error_free(err);
                    gst_message_unref(msg);
                }
                gst_object_unref(bus);
            }

            gst_element_set_state(
                pipeline_,
                GST_STATE_NULL
            );

            return;
        }

        std::println(
            "[GStreamer] Pipeline started successfully."
        );
    }

    static void onNegotiationNeededStatic(GstElement* element, gpointer user_data) {
        auto* self = static_cast<GstPipelineManager*>(user_data);
        self->onNegotiationNeeded(element);
    }

    void onNegotiationNeeded(GstElement* element) {
        if (offer_generated_.exchange(true)) {
            // Already generated an offer, ignore subsequent triggers
            return;
        }
        std::println("[WebRTC] Negotiation needed. Creating offer...");

        // Create an offer promise
        GstPromise* promise = gst_promise_new_with_change_func(
            [](GstPromise* promise, gpointer user_data) {
                auto* self = static_cast<GstPipelineManager*>(user_data);
                self->onCreateOfferFinished(promise);
            },
            this,
            nullptr
        );

        g_signal_emit_by_name(element, "create-offer", nullptr, promise);
    }

    void onCreateOfferFinished(GstPromise* promise) {
        const GstStructure* reply = gst_promise_get_reply(promise);
        GstWebRTCSessionDescription* offer = nullptr;
        //gst_structure_get(reply, "offer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &offer, &offer, nullptr);
        gst_structure_get(reply, "offer", GST_TYPE_WEBRTC_SESSION_DESCRIPTION, &offer, nullptr);
        gst_promise_unref(promise);

        if (!offer) {
            std::println(std::cerr, "[WebRTC Error] Failed to create WebRTC offer description.");
            return;
        }

        // Set local description
        g_signal_emit_by_name(webrtc_, "set-local-description", offer, nullptr);

        gchar* sdp_str = gst_sdp_message_as_text(offer->sdp);
        std::string sdp(sdp_str);
        g_free(sdp_str);

        std::string escaped_sdp = escapeJson(sdp);
        std::string json = std::format(
            "{{\"type\":\"offer\",\"sdp\":\"{}\"}}",
            escaped_sdp
        );
        cached_offer_json_ = json;

        const std::size_t target_id = active_session_id_.load();
        if (target_id > 0) {
            outbound_queue_.push(OutboundMessage{
                .target_session_id = target_id,
                .json_payload = std::move(json)
            });
            std::println("[WebRTC] SDP Offer sent to session #{}", target_id);
        }

        gst_webrtc_session_description_free(offer);
    }

    void handleInboundEvent(InboundEvent evt) {
        if (!webrtc_) return;

        if (evt.type == InboundEvent::Type::RemoteAnswer) {
            std::println("[MediaEngine] Applying Answer from Session #{}", evt.session_id);

            // Simple robust extraction of the sdp field value from JSON
            auto sdp_pos = evt.sdp_or_candidate.find("\"sdp\":\"");
            if (sdp_pos == std::string::npos) {
                std::println(std::cerr, "[MediaEngine Error] Could not find 'sdp' key in answer JSON.");
                return;
            }
            sdp_pos += 7;

            // Find closing quote, accounting for potential escaped characters
            std::string sdp_str;
            bool escaped = false;
            for (std::size_t i = sdp_pos; i < evt.sdp_or_candidate.size(); ++i) {
                char c = evt.sdp_or_candidate[i];
                if (escaped) {
                    if (c == 'n') sdp_str.push_back('\n');
                    else if (c == 'r') sdp_str.push_back('\r');
                    else if (c == '"') sdp_str.push_back('"');
                    else if (c == '\\') sdp_str.push_back('\\');
                    else {
                        sdp_str.push_back('\\');
                        sdp_str.push_back(c);
                    }
                    escaped = false;
                } else if (c == '\\') {
                    escaped = true;
                } else if (c == '"') {
                    break; // End of string value
                } else {
                    sdp_str.push_back(c);
                }
            }

            GstSDPMessage* sdp = nullptr;
            gst_sdp_message_new(&sdp);
            if (gst_sdp_message_parse_buffer(reinterpret_cast<const guint8*>(sdp_str.data()), sdp_str.size(), sdp) != GST_SDP_OK) {
                std::println(std::cerr, "[MediaEngine Error] Failed to parse SDP buffer.");
                gst_sdp_message_free(sdp);
                return;
            }

            GstWebRTCSessionDescription* answer = gst_webrtc_session_description_new(GST_WEBRTC_SDP_TYPE_ANSWER, sdp);
            if (!answer) {
                std::println(std::cerr, "[MediaEngine Error] Failed to create GstWebRTCSessionDescription from answer.");
                return;
            }

            // Create a promise to track completion of setting remote description
            GstPromise* promise = gst_promise_new_with_change_func(
                [](GstPromise* p, gpointer) {
                    const GstStructure* reply = gst_promise_get_reply(p);
                    if (reply) {
                        std::println("[MediaEngine] Remote description promise replied successfully.");
                    }
                    gst_promise_unref(p);
                },
                nullptr, nullptr
            );

            g_signal_emit_by_name(
                webrtc_,
                "set-remote-description",
                answer,
                promise
            );

            gst_webrtc_session_description_free(answer);
            std::println("[MediaEngine] Remote Answer successfully passed to webrtcbin.");
        }
        else if (evt.type == InboundEvent::Type::RemoteCandidate) {
            auto cand_pos = evt.sdp_or_candidate.find("\"candidate\":\"");
            if (cand_pos == std::string::npos) return;
            cand_pos += 13;
            auto cand_end = evt.sdp_or_candidate.find("\"", cand_pos);
            std::string cand_str = evt.sdp_or_candidate.substr(cand_pos, cand_end - cand_pos);

            g_signal_emit_by_name(
                webrtc_,
                "add-ice-candidate",
                evt.mline_index,
                cand_str.c_str()
            );
        }
    }

    static void onIceCandidateStatic(
        GstElement*,
        guint mline_index,
        gchar* candidate,
        gpointer user_data
    ) {
        auto* self =
            static_cast<GstPipelineManager*>(
                user_data
            );

        const std::size_t target_id =
            self->active_session_id_.load();

        if (target_id == 0)
            return;

        std::string json = std::format(
            "{{"
            "\"type\":\"candidate\","
            "\"candidate\":{{"
            "\"candidate\":\"{}\","
            "\"sdpMLineIndex\":{}"
            "}}"
            "}}",
            candidate,
            mline_index
        );

        self->outbound_queue_.push(
            OutboundMessage{
                .target_session_id = target_id,
                .json_payload = std::move(json)
            }
        );
    }
};


