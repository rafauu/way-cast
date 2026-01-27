#!/bin/bash

ROOT="$(pwd)"
#VCPKG_BASE="$ROOT/build/vcpkg_installed/x64-linux-release"
#VCPKG_BASE="$ROOT/build/ninja-multi-vcpkg/vcpkg_installed/x64-linux"

#export GST_REGISTRY_DISABLE=yes
export GST_REGISTRY_UPDATE=1
#export GST_PLUGIN_SCANNER="$VCPKG_ROOT/tools/gstreamer/gst-plugin-scanner"
export LD_LIBRARY_PATH="build/debug/vcpkg_installed/x64-linux-dynamic/debug/lib:$LD_LIBRARY_PATH"
export GST_PLUGIN_SCANNER="build/debug/vcpkg_installed/x64-linux-dynamic/tools/gstreamer/gst-plugin-scanner"
export GST_PLUGIN_PATH="build/debug/vcpkg_installed/x64-linux-dynamic/debug/lib/gstreamer-1.0:build/debug/vcpkg_installed/x64-linux-dynamic/debug/plugins/gstreamer"
export PIPEWIRE_CONFIG_DIR="build/debug/vcpkg_installed/x64-linux-dynamic/share/pipewire"
export SPA_PLUGIN_DIR="build/debug/vcpkg_installed/x64-linux-dynamic/debug/lib/spa-0.2"
#export SPA_PLUGIN_DIR="/usr/lib/x86_64-linux-gnu/spa-0.2"
#export PIPEWIRE_MODULE_DIR="/usr/lib/x86_64-linux-gnu/pipewire-0.3"
export PIPEWIRE_DEBUG=1
export GST_DEBUG=1

cmake --build --preset debug && ./build/debug/app
