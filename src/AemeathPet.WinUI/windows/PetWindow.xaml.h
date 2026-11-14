#pragma once

#include "PetWindow.g.h"
#include "Aemeath/Core/Pet/PetBehaviorStateMachine.h"

namespace winrt::AemeathPet::WinUI::implementation {

struct PetWindow : PetWindowT<PetWindow> {
    PetWindow();

    void Root_PointerPressed(IInspectable const& sender, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& args);
    void Root_PointerMoved(IInspectable const& sender, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& args);
    void Root_PointerReleased(IInspectable const& sender, Microsoft::UI::Xaml::Input::PointerRoutedEventArgs const& args);
    void Root_RightTapped(IInspectable const& sender, Microsoft::UI::Xaml::Input::RightTappedRoutedEventArgs const& args);

    void ChatMenu_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
    void SettingsMenu_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
    void ExitMenu_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);

private:
    bool m_dragging { false };
    Windows::Foundation::Point m_lastPoint {};
    Aemeath::Core::Pet::PetBehaviorStateMachine m_stateMachine;
};

} // namespace winrt::AemeathPet::WinUI::implementation
