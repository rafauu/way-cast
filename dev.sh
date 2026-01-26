#!/bin/bash

ROOT="$(pwd)"
VCPKG_BASE="$ROOT/build/vcpkg_installed/x64-linux-gcc15"

export GST_DEBUG=7
export GST_PLUGIN_PATH="$VCPKG_BASE/plugins/gstreamer:$ROOT/gst_plugins"
export LD_LIBRARY_PATH="$VCPKG_BASE/lib:$LD_LIBRARY_PATH"
#export GST_REGISTRY_UPDATE=1
#export GST_PLUGIN_SCANNER="$VCPKG_ROOT/tools/gstreamer/gst-plugin-scanner"

cmake --build build && ./build/app
