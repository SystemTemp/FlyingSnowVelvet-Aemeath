#pragma once

#include "MainWindow.g.h"

namespace winrt::AemeathPet::WinUI::implementation {

struct MainWindow : MainWindowT<MainWindow> {
    MainWindow();

    void OpenPetButton_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
    void OpenChatButton_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
    void OpenSettingsButton_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);

private:
    Microsoft::UI::Xaml::Window m_petWindow { nullptr };
    Microsoft::UI::Xaml::Window m_chatWindow { nullptr };
    Microsoft::UI::Xaml::Window m_settingsWindow { nullptr };
};

} // namespace winrt::AemeathPet::WinUI::implementation
