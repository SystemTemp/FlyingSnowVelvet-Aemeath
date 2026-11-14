#include "pch.h"
#include "WallPage.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;

namespace winrt::AemeathPet::WinUI::implementation {

WallPage::WallPage() {
    InitializeComponent();
    WallList().Items().Append(box_value(L"留言墙已接入原版论坛 API 设计：forum.fxxr.store/api。当前页面是 UI 骨架，后续绑定 ForumApiClient。"));
}

void WallPage::PostButton_Click(IInspectable const&, RoutedEventArgs const&) {
    auto text = ComposerBox().Text();
    if (text.empty()) {
        return;
    }
    WallList().Items().Append(box_value(L"我：" + std::wstring(text.c_str())));
    ComposerBox().Text(L"");
}

} // namespace winrt::AemeathPet::WinUI::implementation
