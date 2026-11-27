#define NOMINMAX
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>

#include "Win32Impl.hpp"

#include <vector>
#include <string>
#include <format>
#include <cstdint>

namespace media::Win32Impl
{

namespace
{

struct MonitorInfo
{
    HMONITOR handle;
    std::wstring name;
    RECT bounds;
    bool primary;
};

[[nodiscard]] BOOL CALLBACK enumerateMonitor(
    HMONITOR monitor,
    HDC,
    LPRECT,
    LPARAM data
)
{
    auto& monitors = *reinterpret_cast<std::vector<MonitorInfo>*>(data);

    MONITORINFOEXW info{};
    info.cbSize = sizeof(info);

    if (GetMonitorInfoW(monitor, &info))
    {
        monitors.push_back({
            .handle = monitor,
            .name = info.szDevice,
            .bounds = info.rcMonitor,
            .primary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0
        });
    }

    return TRUE;
}

}

std::optional<std::uintptr_t> selectMonitor()
{
    std::vector<MonitorInfo> monitors;

    EnumDisplayMonitors(
        nullptr,
        nullptr,
        enumerateMonitor,
        reinterpret_cast<LPARAM>(&monitors)
    );

    if (monitors.empty())
    {
        return std::nullopt;
    }

    std::vector<std::wstring> labels;
    std::vector<TASKDIALOG_BUTTON> buttons;

    labels.reserve(monitors.size());
    buttons.reserve(monitors.size());

    for (std::size_t i = 0; i < monitors.size(); ++i)
    {
        const auto& monitor = monitors[i];

        labels.push_back(std::format(
            L"{} — {}x{}{}",
            monitor.name,
            monitor.bounds.right - monitor.bounds.left,
            monitor.bounds.bottom - monitor.bounds.top,
            monitor.primary ? L" (Primary)" : L""
        ));

        buttons.push_back({
            .nButtonID = static_cast<int>(i + 1),
            .pszButtonText = labels.back().c_str()
        });
    }

    TASKDIALOGCONFIG config{};
    config.cbSize = sizeof(config);
    config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION;
    config.pszWindowTitle = L"Way-Cast";
    config.pszMainInstruction = L"Select a monitor to share";
    config.pszContent = L"Choose the display you want to capture.";
    config.cRadioButtons = static_cast<UINT>(buttons.size());
    config.pRadioButtons = buttons.data();
    config.nDefaultRadioButton = 1;
    config.dwCommonButtons = TDCBF_OK_BUTTON | TDCBF_CANCEL_BUTTON;

    int selectedButton = 0;
    int selectedRadio = 0;

    const auto result = TaskDialogIndirect(
        &config,
        &selectedButton,
        &selectedRadio,
        nullptr
    );

    if (FAILED(result) or
        selectedButton != IDOK or
        selectedRadio <= 0)
    {
        return std::nullopt;
    }

    const auto index = static_cast<std::size_t>(selectedRadio - 1);

    if (index >= monitors.size())
    {
        return std::nullopt;
    }

    return reinterpret_cast<std::uintptr_t>(
        monitors[index].handle
    );
}

}
