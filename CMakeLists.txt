cmake_minimum_required(VERSION 4.2)

project(waycast LANGUAGES CXX)

# -----------------------------------------------------------------------------
# Build configuration
# -----------------------------------------------------------------------------

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_LINKER_TYPE MOLD)

# -----------------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------------

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(quill CONFIG REQUIRED)

pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_check_modules(GST_APP REQUIRED IMPORTED_TARGET gstreamer-app-1.0)
pkg_check_modules(GST_BASE REQUIRED IMPORTED_TARGET gstreamer-base-1.0)
pkg_check_modules(GST_WEBRTC REQUIRED IMPORTED_TARGET gstreamer-webrtc-1.0)
pkg_check_modules(GST_SDP REQUIRED IMPORTED_TARGET gstreamer-sdp-1.0)
pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0 gio-unix-2.0)

# -----------------------------------------------------------------------------
# Plugins defines
#
# With:
#   VCPKG_TARGET_TRIPLET=x64-linux-release
# this resolves to:
#   build/release/vcpkg_installed/x64-linux-release/
# -----------------------------------------------------------------------------

set(WAYCAST_VCPKG_PREFIX
  "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}"
)

set(WAYCAST_GSTREAMER_PLUGIN_SCANNER
  "${WAYCAST_VCPKG_PREFIX}/tools/gstreamer/gst-plugin-scanner"
)

set(WAYCAST_GSTREAMER_PLUGIN_PATH
  "${WAYCAST_VCPKG_PREFIX}/plugins/gstreamer"
)

set(WAYCAST_WEB_DIR
  "${WAYCAST_VCPKG_PREFIX}/share/gstwebrtc-api/dist"
)

if(UNIX)
  set(WAYCAST_LIBVA_DRIVERS_PATH
    "/usr/lib/x86_64-linux-gnu/dri"
  )
endif()

set(WAYCAST_ENCODER_OVERWRITE
  ""
)

# -----------------------------------------------------------------------------
# GCC std module
# GCC 15 requires std.cc to be registered as a CXX_MODULES file set.
# -----------------------------------------------------------------------------

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  string(REGEX MATCH "^[0-9]+" GCC_MAJOR "${CMAKE_CXX_COMPILER_VERSION}")

  foreach(PREFIX /usr /usr/local)
    if(EXISTS "${PREFIX}/include/c++/${GCC_MAJOR}/bits/std.cc")
      set(GCC_STD_MODULE "${PREFIX}/include/c++/${GCC_MAJOR}/bits/std.cc")
      set(GCC_STD_DIR "${PREFIX}/include/c++/${GCC_MAJOR}/bits")
      break()
    endif()
  endforeach()

  if(NOT GCC_STD_MODULE)
    message(FATAL_ERROR "Could not find std.cc for GCC ${GCC_MAJOR}.")
  endif()
endif()

# -----------------------------------------------------------------------------
# Executable
# -----------------------------------------------------------------------------

add_executable(${PROJECT_NAME}
  src/main.cpp
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  INTERPROCEDURAL_OPTIMIZATION_RELEASE ON
)

target_compile_options(${PROJECT_NAME}
  PRIVATE
    -Wall
    -Wextra
    $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-Wno-interference-size>
    $<$<CONFIG:Release>:-march=native>
)

set(WAYCAST_SOURCES
    src/CapturePipeline.cppm
    src/Environment.cppm
    src/Logger.cppm
    src/MediaRaii.cppm
    src/ScreenCapture.cppm
    src/VideoEncoders.cppm
)

if(WIN32)
  list(APPEND WAYCAST_SOURCES src/windows/Wgc.cppm)
elseif(UNIX)
  list(APPEND WAYCAST_SOURCES src/linux/XdgPortal.cppm)
endif()

target_sources(${PROJECT_NAME}
  PRIVATE
    FILE_SET std_module TYPE CXX_MODULES BASE_DIRS "${GCC_STD_DIR}" FILES ${GCC_STD_MODULE}
    FILE_SET CXX_MODULES TYPE CXX_MODULES FILES ${WAYCAST_SOURCES}
)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    Threads::Threads
    PkgConfig::GSTREAMER
    PkgConfig::GST_APP
    PkgConfig::GST_BASE
    PkgConfig::GST_WEBRTC
    PkgConfig::GST_SDP
    PkgConfig::GIO
    quill::quill
)

# -----------------------------------------------------------------------------
# Pass runtime paths into the application
# -----------------------------------------------------------------------------

target_compile_definitions(${PROJECT_NAME}
  PRIVATE
    WAYCAST_GSTREAMER_PLUGIN_SCANNER=\"${WAYCAST_GSTREAMER_PLUGIN_SCANNER}\"
    WAYCAST_GSTREAMER_PLUGIN_PATH=\"${WAYCAST_GSTREAMER_PLUGIN_PATH}\"
    WAYCAST_WEB_DIR=\"${WAYCAST_WEB_DIR}\"
    WAYCAST_LIBVA_DRIVERS_PATH=\"${WAYCAST_LIBVA_DRIVERS_PATH}\"
    WAYCAST_ENCODER_OVERWRITE=\"${WAYCAST_ENCODER_OVERWRITE}\"
)

# -----------------------------------------------------------------------------
# Configuration summary
# -----------------------------------------------------------------------------

message(STATUS "")
message(STATUS "============================================================")
message(STATUS "WayCast")
message(STATUS "============================================================")
message(STATUS "Build type:          ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler:            ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Compiler version:    ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "vcpkg triplet:       ${VCPKG_TARGET_TRIPLET}")
message(STATUS "vcpkg prefix:        ${WAYCAST_VCPKG_PREFIX}")
message(STATUS "Gst plugin scanner:  ${WAYCAST_GSTREAMER_PLUGIN_SCANNER}")
message(STATUS "Gst plugin path:     ${WAYCAST_GSTREAMER_PLUGIN_PATH}")
message(STATUS "WebRTC web UI:       ${WAYCAST_WEB_DIR}")
message(STATUS "libVA drivers path:  ${WAYCAST_LIBVA_DRIVERS_PATH}")
message(STATUS "Encoder overwrite:   ${WAYCAST_ENCODER_OVERWRITE}")
message(STATUS "============================================================")
message(STATUS "")

