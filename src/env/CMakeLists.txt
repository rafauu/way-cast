add_library(waycast_env)

# -----------------------------------------------------------------------------
# Plugins & Paths Definitions
# -----------------------------------------------------------------------------

set(WAYCAST_GSTREAMER_PLUGIN_SCANNER
  "${WAYCAST_VCPKG_PREFIX}/tools/gstreamer/gst-plugin-scanner"
)

set(WAYCAST_GSTREAMER_PLUGIN_PATH
  "${WAYCAST_VCPKG_PREFIX}/plugins/gstreamer"
)

if(UNIX)
  set(WAYCAST_LIBVA_DRIVERS_PATH
    "/usr/lib/x86_64-linux-gnu/dri"
  )
endif()

# -----------------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------------

target_sources(waycast_env
  PUBLIC
    FILE_SET CXX_MODULES TYPE CXX_MODULES FILES
      Environment.cppm
)

target_link_libraries(waycast_env
  PUBLIC
    stdcxx
    waycast_logger
)

# -----------------------------------------------------------------------------
# Pass runtime paths into the application
# -----------------------------------------------------------------------------

target_compile_definitions(waycast_env
  PRIVATE
    WAYCAST_GSTREAMER_PLUGIN_SCANNER=\"${WAYCAST_GSTREAMER_PLUGIN_SCANNER}\"
    WAYCAST_GSTREAMER_PLUGIN_PATH=\"${WAYCAST_GSTREAMER_PLUGIN_PATH}\"
    WAYCAST_LIBVA_DRIVERS_PATH=\"${WAYCAST_LIBVA_DRIVERS_PATH}\"
)

# -----------------------------------------------------------------------------
# Configuration summary
# -----------------------------------------------------------------------------

message(STATUS "")
message(STATUS "============================================================")
message(STATUS "WayCast Env")
message(STATUS "============================================================")
message(STATUS "Gst plugin scanner:  ${WAYCAST_GSTREAMER_PLUGIN_SCANNER}")
message(STATUS "Gst plugin path:     ${WAYCAST_GSTREAMER_PLUGIN_PATH}")
message(STATUS "libVA drivers path:  ${WAYCAST_LIBVA_DRIVERS_PATH}")
message(STATUS "============================================================")
