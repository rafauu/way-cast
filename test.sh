#!/bin/bash

ROOT="$(pwd)"
VCPKG_BASE="$ROOT/build/vcpkg_installed/x64-linux-release"

export GST_DEBUG=7
export GST_PLUGIN_SCANNER="$VCPKG_BASE/tools/gstreamer/gst-plugin-scanner"
export GST_PLUGIN_PATH="$VCPKG_BASE/plugins/gstreamer:$VCPKG_BASE/lib/gstreamer-1.0"
export LD_LIBRARY_PATH="$VCPKG_BASE/lib:$LD_LIBRARY_PATH"
export GST_REGISTRY_UPDATE=1

GST_DEBUG=7 build/vcpkg_installed/x64-linux-release/tools/gstreamer/gst-launch-1.0 pipewiresrc ! videoconvert ! fakesink
