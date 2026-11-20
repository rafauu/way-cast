add_library(waycast_media)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)

pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  pkg_check_modules(LIBPORTAL REQUIRED IMPORTED_TARGET libportal)
endif()

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

set(WAYCAST_MEDIA_MODULES
  CapturePipeline.cppm
  CaptureSession.cppm
  PipelineBuilder.cppm
  Raii.cppm
  VideoEncoder.cppm
)

set(WAYCAST_MEDIA_SOURCES)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(APPEND WAYCAST_MEDIA_MODULES linux/Xdp.cppm)
elseif(WIN32)
  list(APPEND WAYCAST_MEDIA_MODULES windows/Win32.cppm)
  list(APPEND WAYCAST_MEDIA_SOURCES windows/Win32.cpp windows/Win32Impl.cpp)
endif()

target_sources(waycast_media
  PUBLIC
    FILE_SET CXX_MODULES TYPE CXX_MODULES FILES ${WAYCAST_MEDIA_MODULES}
  PRIVATE
    ${WAYCAST_MEDIA_SOURCES}
)

set(WAYCAST_MEDIA_LIBRARIES
  waycast_logger
  Threads::Threads
  PkgConfig::GSTREAMER
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(APPEND WAYCAST_MEDIA_LIBRARIES PkgConfig::LIBPORTAL)
elseif(WIN32)
  list(APPEND WAYCAST_MEDIA_LIBRARIES runtimeobject comctl32)
endif()

target_link_libraries(waycast_media
  PUBLIC
    ${WAYCAST_MEDIA_LIBRARIES}
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
