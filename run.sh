#!/bin/bash
set -e

BUILD_DIR="$(pwd)/build/release"
VCPKG_DIR="$BUILD_DIR/vcpkg_installed/x64-linux-dynamic"

SCANNER_PATH=$(find "$VCPKG_DIR" -name "gst-plugin-scanner" -type f 2>/dev/null | head -n 1)
if [ -n "$SCANNER_PATH" ]; then
    export GST_PLUGIN_SCANNER="$SCANNER_PATH"
fi

export GST_PLUGIN_PATH="$VCPKG_DIR/lib/gstreamer-1.0:$VCPKG_DIR/plugins/gstreamer"
export GST_PLUGIN_SYSTEM_PATH="$GST_PLUGIN_PATH"
export LD_LIBRARY_PATH="$VCPKG_DIR/lib:$LD_LIBRARY_PATH"

rm -rf ~/.cache/gstreamer-1.0/

echo "[Env Setup] Plugin paths configured successfully. Launching application..."
echo "------------------------------------------------------------------"

export GST_DEBUG="pipewiresrc:6,webrtcbin:4"
exec "$BUILD_DIR/app" "$@"
