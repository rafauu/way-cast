module;

#include <gst/gst.h>

export module MediaRaii;

import std;

template <auto... Fns>
struct CascadingDeleter
{
    void operator()(auto* ptr) const noexcept
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


export namespace MediaRaii
{
    using GMainLoopPtr = std::unique_ptr<GMainLoop,
        CascadingDeleter<
            g_main_loop_unref
        >
    >;

    using GErrorPtr = std::unique_ptr<GError,
        CascadingDeleter<
            g_error_free
        >
    >;

    using GstPipelineElementPtr = std::unique_ptr<GstElement,
        CascadingDeleter<
            setPipelineNull,
            gst_object_unref
        >
    >;

    using GstElementFactoryPtr = std::unique_ptr<GstElementFactory,
        CascadingDeleter<
            gst_object_unref
        >
    >;
}
