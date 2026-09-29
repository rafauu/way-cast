cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_MODULE_STD OFF)

project(WayCast CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GSTREAMER REQUIRED IMPORTED_TARGET gstreamer-1.0)
pkg_check_modules(GST_APP REQUIRED IMPORTED_TARGET gstreamer-app-1.0)
pkg_check_modules(GST_BASE REQUIRED IMPORTED_TARGET gstreamer-base-1.0)
pkg_check_modules(GST_WEBRTC REQUIRED IMPORTED_TARGET gstreamer-webrtc-1.0)
pkg_check_modules(GST_SDP REQUIRED IMPORTED_TARGET gstreamer-sdp-1.0)
pkg_check_modules(GIO REQUIRED IMPORTED_TARGET gio-2.0 gio-unix-2.0)

add_library(app_modules)

target_sources(app_modules PUBLIC
    FILE_SET CXX_MODULES FILES
        src/compat/std_module.cppm # bug in cmake3/gcc15
        src/MpscQueue.ixx
        src/AppConfig.ixx
        src/WaylandPortal.ixx
        src/GstPipelineManager.ixx
)

target_compile_options(app_modules PRIVATE -fmodules-ts)

target_link_directories(app_modules PRIVATE
    ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux-dynamic/lib
)

target_link_libraries(app_modules PRIVATE
    PkgConfig::GSTREAMER
    PkgConfig::GST_APP
    PkgConfig::GST_BASE
    PkgConfig::GST_WEBRTC
    PkgConfig::GST_SDP
    PkgConfig::GIO
)

add_executable(app src/main.cpp)

target_link_directories(app PRIVATE
    ${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux-dynamic/lib
)

target_link_libraries(app PRIVATE
    app_modules
    PkgConfig::GIO
)

target_link_options(app PRIVATE "-Wl,--export-dynamic")
