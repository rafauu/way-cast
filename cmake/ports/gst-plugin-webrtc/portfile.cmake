set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)
set(VCPKG_POLICY_MISMATCHED_NUMBER_OF_BINARIES enabled)

vcpkg_from_gitlab(
    OUT_SOURCE_PATH SOURCE_PATH
    GITLAB_URL https://gitlab.freedesktop.org
    REPO gstreamer/gst-plugins-rs
    REF 0.15.3
    SHA512 ed6650bf1d17b91452fb00f57659a482cb544b5cb4032d6bf523d90d3817a5904b1d7e75d21c1dabf916ef28279ad9b72a807ba49827cd457a6161119d4bfece
)

find_program(CARGO_EXECUTABLE cargo REQUIRED)
find_program(NPM_EXECUTABLE npm REQUIRED)

set(ENV{PKG_CONFIG_PATH}
    "${CURRENT_INSTALLED_DIR}/lib/pkgconfig:${CURRENT_INSTALLED_DIR}/share/pkgconfig"
)

set(ENV{PKG_CONFIG_ALLOW_SYSTEM_CFLAGS} 1)
set(ENV{PKG_CONFIG_ALLOW_SYSTEM_LIBS} 1)

# --------------------------------------------------------------------------
# Build gstwebrtc-api
# --------------------------------------------------------------------------

set(GST_WEBRTC_API_DIR
    "${SOURCE_PATH}/net/webrtc/gstwebrtc-api"
)

if(NOT EXISTS "${GST_WEBRTC_API_DIR}/package.json")
    message(FATAL_ERROR
        "gstwebrtc-api package.json was not found at "
        "${GST_WEBRTC_API_DIR}"
    )
endif()

vcpkg_execute_required_process(
    COMMAND
        "${NPM_EXECUTABLE}"
        install
    WORKING_DIRECTORY "${GST_WEBRTC_API_DIR}"
    LOGNAME gst-webrtc-api-npm-install
)

vcpkg_execute_required_process(
    COMMAND
        "${NPM_EXECUTABLE}"
        run
        build
    WORKING_DIRECTORY "${GST_WEBRTC_API_DIR}"
    LOGNAME gst-webrtc-api-build
)

set(GST_WEBRTC_API_DIST
    "${GST_WEBRTC_API_DIR}/dist"
)

if(NOT EXISTS "${GST_WEBRTC_API_DIST}/index.html")
    message(FATAL_ERROR
        "gstwebrtc-api build completed but index.html was not found: "
        "${GST_WEBRTC_API_DIST}"
    )
endif()

message(STATUS
    "gstwebrtc-api generated successfully: "
    "${GST_WEBRTC_API_DIST}"
)

# --------------------------------------------------------------------------
# Build GStreamer Rust WebRTC plugin
# --------------------------------------------------------------------------

vcpkg_execute_required_process(
    COMMAND
        "${CARGO_EXECUTABLE}"
        build
        --release
        -p
        gst-plugin-rtp
        -p
        gst-plugin-webrtc
    WORKING_DIRECTORY "${SOURCE_PATH}"
    LOGNAME gst-plugin-webrtc-build
)

set(GST_RTP_PLUGIN
    "${SOURCE_PATH}/target/release/libgstrsrtp.so"
)

set(GST_WEBRTC_PLUGIN
    "${SOURCE_PATH}/target/release/libgstrswebrtc.so"
)

if(NOT EXISTS "${GST_WEBRTC_PLUGIN}")
    message(FATAL_ERROR
        "gst-plugin-webrtc build completed but "
        "${GST_WEBRTC_PLUGIN} was not found"
    )
endif()

# --------------------------------------------------------------------------
# Install plugin
# --------------------------------------------------------------------------

file(
    INSTALL
        "${GST_RTP_PLUGIN}"
        "${GST_WEBRTC_PLUGIN}"
    DESTINATION
        "${CURRENT_PACKAGES_DIR}/lib/gstreamer-1.0"
)

# --------------------------------------------------------------------------
# Install web application
# --------------------------------------------------------------------------

file(
    INSTALL
        "${GST_WEBRTC_API_DIST}/"
    DESTINATION
        "${CURRENT_PACKAGES_DIR}/share/gstwebrtc-api/dist"
)

# --------------------------------------------------------------------------
# License
# --------------------------------------------------------------------------

vcpkg_install_copyright(
    FILE_LIST
        "${SOURCE_PATH}/LICENSE-MIT"
        "${SOURCE_PATH}/LICENSE-APACHE"
        "${GST_WEBRTC_API_DIR}/LICENSE-MPL-2.0"
)

