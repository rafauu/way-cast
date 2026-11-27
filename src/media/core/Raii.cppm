module;

#include <gst/gst.h>
#ifdef __linux__
#include <libportal/portal.h>
#include <libportal/session.h>
#endif

export module media.core.raii;

import std;

namespace media::core::raii::detail
{

template <auto... Fns>
struct CascadingDeleter
{
    static void operator()(auto* ptr) noexcept
    {
        if (ptr)
        {
            (Fns(ptr), ...);
        }
    }
};

constexpr auto setPipelineNull = [] (auto* ptr) {
    gst_element_set_state(ptr, GST_STATE_NULL);
};

}

export namespace media::core::raii
{
    using GMainLoopPtr = std::unique_ptr<GMainLoop,
        detail::CascadingDeleter<
            g_main_loop_unref
        >
    >;

    using GErrorPtr = std::unique_ptr<GError,
        detail::CascadingDeleter<
            g_error_free
        >
    >;

    using GstPipelineElementPtr = std::unique_ptr<GstElement,
        detail::CascadingDeleter<
            detail::setPipelineNull,
            gst_object_unref
        >
    >;

    using GstElementFactoryPtr = std::unique_ptr<GstElementFactory,
        detail::CascadingDeleter<
            gst_object_unref
        >
    >;

#ifdef __linux__
    using XdpSessionPtr = std::unique_ptr<XdpSession,
        detail::CascadingDeleter<
            g_object_unref
        >
    >;
#endif
}
