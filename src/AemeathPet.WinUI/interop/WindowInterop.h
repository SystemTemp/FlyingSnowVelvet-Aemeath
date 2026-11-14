#pragma once

#include <windows.h>
#include <winrt/Microsoft.UI.Xaml.h>

namespace AemeathPet::WinUI::Interop {

class WindowInterop final {
public:
    static HWND GetHwnd(winrt::Microsoft::UI::Xaml::Window const& window);
    static void SetTopMost(HWND hwnd, bool enabled);
    static void SetToolWindow(HWND hwnd, bool enabled);
    static void SetClickThrough(HWND hwnd, bool enabled);
    static void SetNoBorder(HWND hwnd);
};

} // namespace AemeathPet::WinUI::Interop
