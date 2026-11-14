#include "pch.h"
#include "PetWindow.xaml.h"
#include "../interop/WindowInterop.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Input;

namespace winrt::AemeathPet::WinUI::implementation {

PetWindow::PetWindow() {
    InitializeComponent();

    // Windows 端启用：获取 HWND 后设置无边框、置顶、工具窗口、透明背景。
    // auto hwnd = WindowInterop::GetHwnd(*this);
    // WindowInterop::SetTopMost(hwnd, true);
    // WindowInterop::SetToolWindow(hwnd, true);
}

void PetWindow::Root_PointerPressed(IInspectable const&, PointerRoutedEventArgs const& args) {
    m_dragging = true;
    m_lastPoint = args.GetCurrentPoint(Root()).Position();
    m_stateMachine.ToDragging();
}

void PetWindow::Root_PointerMoved(IInspectable const&, PointerRoutedEventArgs const& args) {
    if (!m_dragging) {
        return;
    }

    const auto current = args.GetCurrentPoint(Root()).Position();
    const auto dx = current.X - m_lastPoint.X;
    const auto dy = current.Y - m_lastPoint.Y;

    // WinUI AppWindow/Win32 MoveWindow 接口在 Windows 端补齐。
    (void)dx;
    (void)dy;
}

void PetWindow::Root_PointerReleased(IInspectable const&, PointerRoutedEventArgs const&) {
    m_dragging = false;
    m_stateMachine.ToIdle();
}

void PetWindow::Root_RightTapped(IInspectable const&, RightTappedRoutedEventArgs const& args) {
    PetMenu().ShowAt(Root(), args.GetPosition(Root()));
}

void PetWindow::ChatMenu_Click(IInspectable const&, RoutedEventArgs const&) {
    m_stateMachine.ToTalking();
}

void PetWindow::SettingsMenu_Click(IInspectable const&, RoutedEventArgs const&) {
    // TODO: 发布打开设置事件，或由窗口管理器打开 SettingsWindow。
}

void PetWindow::ExitMenu_Click(IInspectable const&, RoutedEventArgs const&) {
    Close();
}

} // namespace winrt::AemeathPet::WinUI::implementation
