#include "pch.h"
#include "MainWindow.xaml.h"
#include "windows/PetWindow.xaml.h"
#include "windows/ChatWindow.xaml.h"
#include "windows/SettingsWindow.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::AemeathPet::WinUI::implementation {

MainWindow::MainWindow() {
    InitializeComponent();
}

void MainWindow::OpenPetButton_Click(IInspectable const&, RoutedEventArgs const&) {
    if (!m_petWindow) {
        m_petWindow = make<PetWindow>();
    }
    m_petWindow.Activate();
}

void MainWindow::OpenChatButton_Click(IInspectable const&, RoutedEventArgs const&) {
    if (!m_chatWindow) {
        m_chatWindow = make<ChatWindow>();
    }
    m_chatWindow.Activate();
}

void MainWindow::OpenSettingsButton_Click(IInspectable const&, RoutedEventArgs const&) {
    if (!m_settingsWindow) {
        m_settingsWindow = make<SettingsWindow>();
    }
    m_settingsWindow.Activate();
}

} // namespace winrt::AemeathPet::WinUI::implementation
