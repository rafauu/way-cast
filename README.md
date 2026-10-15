# way-cast
## Wayland screen caster

### Prerequisites
```
sudo apt install git cmake ninja-build gcc mold nasm bison flex autoconf autoconf-archive automake libtool python3-venv cargo rustc npm
[optional] sudo apt install intel-media-va-driver libdrm-dev (for vaapi hw encoding)
```

### Known issues
When performing real-time screen sharing or capturing on GNOME (Wayland) via PipeWire, full-screen windows or heavy applications can trigger a display optimization known as **Direct Scanout**.


Direct Scanout allows the display server (Mutter) to bypass compositing entirely and send full-screen buffers directly to the display hardware to save resources and lower latency. However, PipeWire relies on the compositor's frame graph to capture streams. When direct scanout activates, the compositor stops tracking those frames, causing the WebRTC stream to experience black screens, freezes, or flickering when applications go full-screen.

#### The Solution
Setting the environment variable `MUTTER_DEBUG_PAINT=disable-direct-scanout` forces Mutter to keep compositing active at all times. This ensures that PipeWire can consistently intercept and capture smooth, uninterrupted frame buffers for real-time streaming without dropping full-screen windows.
```
mkdir -p ~/.config/environment.d
echo "MUTTER_DEBUG_PAINT=disable-direct-scanout" > ~/.config/environment.d/90-mutter-debug.conf
```
*Session restart required*

The same issue appears on KDE (Wayland):
```
echo "KWIN_DRM_NO_DIRECT_SCANOUT=1" > ~/.config/environment.d/90-kwin-debug.conf
```

### Build & run
```
git clone --recurse-submodules git@github.com:rafauu/way-cast.git
cmake --preset release
cmake --build --preset release
./build/release/waycast
[browser] http://localhost:8080/ or http://<host_ip>:8080/
          Remote streams -> waycast
```
