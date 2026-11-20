# way-cast

## Native screen caster

### Description
**way-cast** is a high-performance, native C++ screen-casting engine for Windows and Linux (Wayland). It captures screen and audio streams using platform-native technologies, leveraging **XDG Desktop Portal, PipeWire** on Linux and **Win32, Direct3D 11, WASAPI** on Windows. Video is encoded using hardware acceleration (**NVENC, VA-API, Media Foundation**) or software encoding (**x264**), with **Opus** for audio. Powered by GStreamer's `webrtcsink`, **way-cast** streams low-latency, high-framerate audio and video directly to web browsers over WebRTC, with a built-in HTTP and signaling server.

### Prerequisites
#### Linux
```
sudo apt install git cmake ninja-build gcc nasm bison flex autoconf autoconf-archive automake libtool python3-venv cargo rustc npm libportal-dev
[optional] sudo apt install intel-media-va-driver libdrm-dev (for vaapi hw encoding)
```
#### Windows (MSYS2/UCRT64)
```
Install WinSdk (for mediafoundation plugin) from https://learn.microsoft.com/pl-pl/windows/apps/windows-sdk/downloads
export WindowsSDKDir="C:/Program Files (x86)/Windows Kits/10"
export WindowsSDKVersion="10.0.20348.0"
export VCPKG_KEEP_ENV_VARS="WindowsSDKDir;WindowsSDKVersion"

pacman -Syu
pacman -S --needed git mingw-w64-ucrt-x86_64-{cmake,ninja,gcc,mold,rust,nodejs}
```
### Build & run
```
git clone --recurse-submodules git@github.com:rafauu/way-cast.git
cmake --preset linux-release [or windows-release]
cmake --build --preset linux-release
./build/linux-release/waycast
[browser] http://localhost:8080/ or http://<host_ip>:8080/
          Remote streams -> waycast
```

### Known issues
#### Direct Scanout on Wayland Compositors
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
#### Windows Firewall
It might be required to allow traffic and change network category.
```
New-NetFirewallRule `
  -DisplayName "WayCast Web Server" `
  -Direction Inbound `
  -Protocol TCP `
  -LocalPort 8080 `
  -Action Allow `
  -Profile Private

New-NetFirewallRule `
  -DisplayName "WayCast Signalling Server" `
  -Direction Inbound `
  -Protocol TCP `
  -LocalPort 8443 `
  -Action Allow `
  -Profile Private
```
Depending on chosen allowed network profiles for this app at startup, network category may require a change, e.g. if allowed profile is Private only and `Get-NetConnectionProfile` yields Public network category, you need to change it:
```
Set-NetConnectionProfile `
  -InterfaceAlias "Wi-Fi" `
  -NetworkCategory Private
```
