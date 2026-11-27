add_library(waycast_env)

# -----------------------------------------------------------------------------
# Configuration
# -----------------------------------------------------------------------------

set(WAYCAST_VCPKG_BIN
  "${WAYCAST_VCPKG_PREFIX}/bin"
)

set(WAYCAST_GSTREAMER_PLUGIN_SCANNER
  "${WAYCAST_VCPKG_PREFIX}/tools/gstreamer/gst-plugin-scanner"
)

set(WAYCAST_GSTREAMER_PLUGIN_PATH
  "${WAYCAST_VCPKG_PREFIX}/plugins/gstreamer"
)

set(WAYCAST_GSTREAMER_RUST_PLUGIN_PATH
  "${WAYCAST_VCPKG_PREFIX}/bin/gstreamer-1.0"
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
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

# -----------------------------------------------------------------------------
# Linking
# -----------------------------------------------------------------------------

target_link_libraries(waycast_env
  PRIVATE
    waycast_logger
)

# -----------------------------------------------------------------------------
# Compile definitions
# -----------------------------------------------------------------------------

target_compile_definitions(waycast_env
  PRIVATE
    WAYCAST_VCPKG_BIN=\"${WAYCAST_VCPKG_BIN}\"
    WAYCAST_GSTREAMER_PLUGIN_SCANNER=\"${WAYCAST_GSTREAMER_PLUGIN_SCANNER}\"
    WAYCAST_GSTREAMER_PLUGIN_PATH=\"${WAYCAST_GSTREAMER_PLUGIN_PATH}\"
    WAYCAST_GSTREAMER_RUST_PLUGIN_PATH=\"${WAYCAST_GSTREAMER_RUST_PLUGIN_PATH}\"
    WAYCAST_LIBVA_DRIVERS_PATH=\"${WAYCAST_LIBVA_DRIVERS_PATH}\"
)

# -----------------------------------------------------------------------------
# Configuration summary
# -----------------------------------------------------------------------------

message(STATUS "")
message(STATUS "============================================================")
message(STATUS "WayCast Env")
message(STATUS "============================================================")
message(STATUS "Vcpkg bin:            ${WAYCAST_VCPKG_BIN}")
message(STATUS "Gst plugin scanner:   ${WAYCAST_GSTREAMER_PLUGIN_SCANNER}")
message(STATUS "Gst plugin path:      ${WAYCAST_GSTREAMER_PLUGIN_PATH}")
message(STATUS "Gst rust plugin path: ${WAYCAST_GSTREAMER_RUST_PLUGIN_PATH}")
message(STATUS "libVA drivers path:   ${WAYCAST_LIBVA_DRIVERS_PATH}")
message(STATUS "============================================================")
