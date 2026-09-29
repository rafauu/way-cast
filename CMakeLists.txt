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

pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_check_modules(GST_APP REQUIRED IMPORTED_TARGET gstreamer-app-1.0)
pkg_check_modules(GST_BASE REQUIRED IMPORTED_TARGET gstreamer-base-1.0)
pkg_check_modules(GST_WEBRTC REQUIRED IMPORTED_TARGET gstreamer-webrtc-1.0)
pkg_check_modules(GST_SDP REQUIRED IMPORTED_TARGET gstreamer-sdp-1.0)
pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0 gio-unix-2.0)

# -----------------------------------------------------------------------------
# vcpkg paths
#
# With:
#
#   VCPKG_TARGET_TRIPLET=x64-linux-release
#
# this resolves to:
#
#   build/release/vcpkg_installed/x64-linux-release/
# -----------------------------------------------------------------------------

if(NOT VCPKG_TARGET_TRIPLET)
    message(FATAL_ERROR
        "VCPKG_TARGET_TRIPLET is not set."
    )
endif()

set(WAYCAST_VCPKG_PREFIX
    "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}"
)

set(WAYCAST_VCPKG_LIB_DIR
    "${WAYCAST_VCPKG_PREFIX}/lib"
)

# Standard vcpkg GStreamer plugins.
#
# Example:
#   libgstrtp.so
set(WAYCAST_GSTREAMER_PLUGIN_DIR
    "${WAYCAST_VCPKG_PREFIX}/plugins/gstreamer"
)

# GStreamer Rust plugins installed by gst-plugin-webrtc.
#
# Example:
#   libgstrswebrtc.so
#   libgstrsrtp.so
set(WAYCAST_GSTREAMER_RUST_PLUGIN_DIR
    "${WAYCAST_VCPKG_PREFIX}/lib/gstreamer-1.0"
)

# gstwebrtc-api web frontend installed by the overlay.
set(WAYCAST_WEB_DIR
    "${WAYCAST_VCPKG_PREFIX}/share/gstwebrtc-api/dist"
)

# -----------------------------------------------------------------------------
# GCC std module
#
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
# WayCast modules
# -----------------------------------------------------------------------------

add_library(waycast_modules)

target_sources(waycast_modules
    PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS
            "${GCC_STD_DIR}"
            "${CMAKE_CURRENT_SOURCE_DIR}/src"
        FILES
            "${GCC_STD_MODULE}"
            src/WaylandPortal.cppm
            src/GstPipelineManager.cppm
)

target_compile_features(
    waycast_modules
    PUBLIC
        cxx_std_26
)

target_compile_options(
    waycast_modules

    PRIVATE

        -Wall
        -Wextra

        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
            -Wno-interference-size
        >

        $<$<CONFIG:Release>:
            -march=native
        >
)

target_link_libraries(
    waycast_modules

    PRIVATE

        Threads::Threads

        PkgConfig::GSTREAMER
        PkgConfig::GST_APP
        PkgConfig::GST_BASE
        PkgConfig::GST_WEBRTC
        PkgConfig::GST_SDP
        PkgConfig::GIO
)

# -----------------------------------------------------------------------------
# Executable
# -----------------------------------------------------------------------------

add_executable(
    waycast
    src/main.cpp
)

target_compile_features(
    waycast
    PRIVATE
        cxx_std_26
)

target_compile_options(
    waycast

    PRIVATE

        -Wall
        -Wextra

        $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:
            -Wno-interference-size
        >

        $<$<CONFIG:Release>:
            -march=native
        >
)

target_link_libraries(
    waycast
    PRIVATE
        waycast_modules
        Threads::Threads
        PkgConfig::GIO
)

# -----------------------------------------------------------------------------
# Runtime shared-library search path
# -----------------------------------------------------------------------------

# This is for shared libraries such as:
#
#   libgstreamer-1.0.so
#   libpipewire.so
#   libnice.so
#   etc.
#
# It does NOT replace GST_PLUGIN_PATH.
#set_target_properties(
#    waycast
#
#    PROPERTIES
#
#        BUILD_RPATH
#            "${WAYCAST_VCPKG_LIB_DIR}"
#
#        INSTALL_RPATH
#            "\$ORIGIN/../lib"
#)

# -----------------------------------------------------------------------------
# Pass runtime paths into the application
# -----------------------------------------------------------------------------

# These replace hard-coded paths in GstPipelineManager.cppm.
target_compile_definitions(
    waycast_modules
    PRIVATE
        WAYCAST_GSTREAMER_PLUGIN_DIR="${WAYCAST_GSTREAMER_PLUGIN_DIR}"
        WAYCAST_GSTREAMER_RUST_PLUGIN_DIR="${WAYCAST_GSTREAMER_RUST_PLUGIN_DIR}"
        WAYCAST_WEB_DIR="${WAYCAST_WEB_DIR}"
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
message(STATUS "GStreamer plugins:   ${WAYCAST_GSTREAMER_PLUGIN_DIR}")
message(STATUS "GStreamer Rust:      ${WAYCAST_GSTREAMER_RUST_PLUGIN_DIR}")
message(STATUS "WebRTC web UI:       ${WAYCAST_WEB_DIR}")
message(STATUS "============================================================")
message(STATUS "")

