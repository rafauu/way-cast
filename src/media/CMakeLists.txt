add_library(waycast_media)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)

# -----------------------------------------------------------------------------
# Configuration
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

target_sources(waycast_media
  PUBLIC
    FILE_SET CXX_MODULES TYPE CXX_MODULES FILES
      CapturePipeline.cppm
      CaptureSession.cppm
      PipelineBuilder.cppm
      VideoEncoder.cppm
)

# -----------------------------------------------------------------------------
# Components
# -----------------------------------------------------------------------------

add_subdirectory(core)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_subdirectory(backend/linux)
elseif(WIN32)
  add_subdirectory(backend/windows)
endif()

# -----------------------------------------------------------------------------
# Linking
# -----------------------------------------------------------------------------

target_link_libraries(waycast_media
  PRIVATE
    PkgConfig::GSTREAMER
    waycast_media_core
    waycast_media_backend
)

# -----------------------------------------------------------------------------
# Compile definitions
# -----------------------------------------------------------------------------

target_compile_definitions(waycast_media
  PRIVATE
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
