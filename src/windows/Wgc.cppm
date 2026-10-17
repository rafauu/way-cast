module;

#define NOMINMAX
#include <windows.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.Capture.h>

export module Wgc;

import Logger;
import std;

export class Wgc
{
public:
    struct SessionData
    {
        std::uint64_t windowHandle{0};
    };

    static std::optional<SessionData> requestScreenCapture()
    {
        Logger::info("[Wgc] Opening native Windows Graphics Capture picker.");

        try
        {
            namespace WGC = winrt::Windows::Graphics::Capture;

            if (not WGC::GraphicsCaptureSession::IsSupported())
            {
                Logger::error("[Wgc] WGC is not supported on this OS version.");
                return std::nullopt;
            }

            WGC::GraphicsCapturePicker picker;
            auto item = picker.PickSingleItemAsync().get();

            if (!item)
            {
                Logger::warn("[Wgc] User cancelled the screen capture selection.");
                return std::nullopt;
            }

            Logger::info("[Wgc] Screen capture item successfully selected.");

            return SessionData{
                .windowHandle = reinterpret_cast<std::uint64_t>(winrt::get_abi(item))
            };
        }
        catch (const winrt::hresult_error& ex)
        {
            Logger::error("[Wgc] WinRT error: {}", winrt::to_string(ex.message()));
            return std::nullopt;
        }
        catch (const std::exception& ex)
        {
            Logger::error("[Wgc] Standard exception: {}", ex.what());
            return std::nullopt;
        }
    }
}
