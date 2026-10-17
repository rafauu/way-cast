# way-cast

## Native screen caster

### Description
`way-cast` is a high-performance, native C++ screen-casting engine designed for Linux Wayland environments. It securely requests screen-capture sessions via **XDG Desktop Portals** (`xdg-desktop-portal`), receives high-framerate video and audio streams through **PipeWire**, encodes them via hardware acceleration (VA-API / NVENC) or software (`x264`/`opus`), and streams them in real-time to web browsers using **GStreamer's `webrtcsink`** (bundled with a built-in signaling and web server).

### Architecture Flow
1. **XDG Desktop Portal (`XdgPortal`)**: Prompts the user with a system dialog to select a monitor/window securely, returning a PipeWire file descriptor (`fd`) and node ID.
2. **PipeWire (`pipewiresrc`)**: Captures zero-copy raw video and audio streams directly from the display server's processing graph.
3. **Encoding & Processing**: Converts and encodes raw frames using the best available hardware/software encoder (`nvh264enc`, `vah264enc`, or `x264enc`) alongside Opus audio encoding.
4. **WebRTC (`webrtcsink`)**: Publishes the encoded streams over WebRTC, hosting a local HTTP and signaling server to let clients view the stream instantly in a browser.

### Prerequisites
```
sudo apt install git cmake ninja-build gcc mold nasm bison flex autoconf autoconf-archive automake libtool python3-venv cargo rustc npm
[optional] sudo apt install intel-media-va-driver libdrm-dev (for vaapi hw encoding)
```

### Known issues: Direct Scanout on Wayland Compositors
When performing real-time screen capture on Wayland via PipeWire, full-screen windows or heavy 3D applications can trigger a display optimization known as **Direct Scanout**.

- What happens: Direct Scanout allows the Wayland compositor (Mutter on GNOME or KWin on KDE) to bypass the compositor's rendering pipeline entirely, handing the application's client buffer straight to the display controller hardware to minimize latency and save power.

- The Problem for Capture: Because the buffer bypasses the compositor's internal composition pass, screen-casting portals and recorders sampling from the compositor often receive blank frames, black screens, or frozen content while the application is maximized or full-screen.

- The Solution: Setting the environment variable `MUTTER_DEBUG_PAINT=disable-direct-scanout` forces GNOME's Mutter compositor to keep regular compositing active. This ensures that PipeWire can consistently access and intercept valid frame buffers for uninterrupted real-time streaming
```
mkdir -p ~/.config/environment.d
echo "MUTTER_DEBUG_PAINT=disable-direct-scanout" > ~/.config/environment.d/90-mutter-debug.conf
```
*(A user session restart is required for environment.d changes to take effect)*

For KDE Plasma (Wayland):
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
