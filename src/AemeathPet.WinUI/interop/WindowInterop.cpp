#include "pch.h"
#include "WindowInterop.h"
#include <microsoft.ui.xaml.window.h>

namespace AemeathPet::WinUI::Interop {

HWND WindowInterop::GetHwnd(winrt::Microsoft::UI::Xaml::Window const& window) {
    auto native = window.as<IWindowNative>();
    HWND hwnd {};
    native->get_WindowHandle(&hwnd);
    return hwnd;
}

void WindowInterop::SetTopMost(HWND hwnd, bool enabled) {
    SetWindowPos(
        hwnd,
        enabled ? HWND_TOPMOST : HWND_NOTOPMOST,
        0,
        0,
        0,
        0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

void WindowInterop::SetToolWindow(HWND hwnd, bool enabled) {
    auto exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    if (enabled) {
        exStyle |= WS_EX_TOOLWINDOW;
        exStyle &= ~WS_EX_APPWINDOW;
    } else {
        exStyle &= ~WS_EX_TOOLWINDOW;
    }
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
}

void WindowInterop::SetClickThrough(HWND hwnd, bool enabled) {
    auto exStyle = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    if (enabled) {
        exStyle |= WS_EX_TRANSPARENT | WS_EX_LAYERED;
    } else {
        exStyle &= ~WS_EX_TRANSPARENT;
    }
    SetWindowLongPtr(hwnd, GWL_EXSTYLE, exStyle);
}

void WindowInterop::SetNoBorder(HWND hwnd) {
    auto style = GetWindowLongPtr(hwnd, GWL_STYLE);
    style &= ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU);
    SetWindowLongPtr(hwnd, GWL_STYLE, style);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
}

} // namespace AemeathPet::WinUI::Interop
