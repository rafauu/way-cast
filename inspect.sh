#!/bin/bash

ROOT="$(pwd)"
VCPKG_BASE="$ROOT/build/vcpkg_installed/x64-linux-gcc15"

export GST_DEBUG=7
export GST_PLUGIN_SCANNER="$VCPKG_BASE/tools/gstreamer/gst-plugin-scanner"
export GST_PLUGIN_PATH="$VCPKG_BASE/plugins/gstreamer:$ROOT/gst_plugins"
export LD_LIBRARY_PATH="$VCPKG_BASE/lib:$LD_LIBRARY_PATH"
#export GST_REGISTRY_UPDATE=1

#./vcpkg/packages/gstreamer_x64-linux-gcc15/tools/gstreamer/gst-inspect-1.0 /home/rafal/hls-cast/build/vcpkg_installed/x64-linux-gcc15/lib/gstreamer-1.0/libgstpipewire.so
./build/vcpkg_installed/x64-linux-gcc15/tools/gstreamer/gst-inspect-1.0 | grep sink | grep video
