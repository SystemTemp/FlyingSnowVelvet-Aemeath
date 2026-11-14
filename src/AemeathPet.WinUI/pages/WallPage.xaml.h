#pragma once

#include "WallPage.g.h"

namespace winrt::AemeathPet::WinUI::implementation {

struct WallPage : WallPageT<WallPage> {
    WallPage();
    void PostButton_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);
};

} // namespace winrt::AemeathPet::WinUI::implementation
