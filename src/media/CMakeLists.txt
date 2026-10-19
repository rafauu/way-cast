add_library(waycast_media)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_check_modules(LIBPORTAL REQUIRED IMPORTED_TARGET libportal)

# -----------------------------------------------------------------------------
# Plugins & Paths Definitions
# -----------------------------------------------------------------------------

set(WAYCAST_WEB_DIR
  "${WAYCAST_VCPKG_PREFIX}/share/gstwebrtc-api/dist"
)

set(WAYCAST_ENCODER_OVERWRITE
  ""
)

# -----------------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------------

set(WAYCAST_MEDIA_SOURCES
    CapturePipeline.cppm
    Raii.cppm
    ScreenCapture.cppm
    VideoEncoders.cppm
)

if(UNIX)
  list(APPEND WAYCAST_MEDIA_SOURCES linux/Xdp.cppm)
elseif(WIN32)
  list(APPEND WAYCAST_MEDIA_SOURCES windows/Wgc.cppm)
endif()

target_sources(waycast_media
  PUBLIC
    FILE_SET CXX_MODULES TYPE CXX_MODULES FILES ${WAYCAST_MEDIA_SOURCES}
)

target_link_libraries(waycast_media
  PUBLIC
    stdcxx
    waycast_logger
    Threads::Threads
    PkgConfig::GSTREAMER
    PkgConfig::LIBPORTAL
)

# -----------------------------------------------------------------------------
# Pass runtime paths into the application
# -----------------------------------------------------------------------------

target_compile_definitions(waycast_media
  PUBLIC
    WAYCAST_WEB_DIR=\"${WAYCAST_WEB_DIR}\"
    WAYCAST_ENCODER_OVERWRITE=\"${WAYCAST_ENCODER_OVERWRITE}\"
)

# -----------------------------------------------------------------------------
# Configuration summary
# -----------------------------------------------------------------------------

message(STATUS "")
message(STATUS "============================================================")
message(STATUS "WayCast Media")
message(STATUS "============================================================")
message(STATUS "WebRTC web UI:       ${WAYCAST_WEB_DIR}")
message(STATUS "Encoder overwrite:   ${WAYCAST_ENCODER_OVERWRITE}")
message(STATUS "============================================================")
