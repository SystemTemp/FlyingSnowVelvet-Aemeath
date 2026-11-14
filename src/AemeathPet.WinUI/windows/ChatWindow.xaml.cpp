#include "pch.h"
#include "ChatWindow.xaml.h"

using namespace winrt;
using namespace Microsoft::UI::Xaml;
using namespace Microsoft::UI::Xaml::Controls;

namespace winrt::AemeathPet::WinUI::implementation {

namespace {
std::string ToUtf8(hstring const& text) {
    return winrt::to_string(text);
}

std::wstring ToWide(std::string const& text) {
    return winrt::to_hstring(text).c_str();
}
}

ChatWindow::ChatWindow() {
    InitializeComponent();
    AppendMessage(L"系统：当前为 Mock 聊天，后续可替换为 OpenAI 兼容接口。 ");
}

void ChatWindow::SendButton_Click(IInspectable const&, RoutedEventArgs const&) {
    auto text = InputBox().Text();
    if (text.empty()) {
        return;
    }

    AppendMessage(L"你：" + std::wstring(text.c_str()));

    Aemeath::Core::Chat::ChatMessage user;
    user.id = "user-message";
    user.role = Aemeath::Core::Chat::ChatRole::User;
    user.content = ToUtf8(text);
    m_session.Add(user);

    auto future = m_chatService.SendAsync(m_session.Messages(), user);
    auto reply = future.get();
    m_session.Add(reply);
    AppendMessage(L"雪绒：" + ToWide(reply.content));

    InputBox().Text(L"");
}

void ChatWindow::AppendMessage(std::wstring const& text) {
    MessageList().Items().Append(box_value(text));
}

} // namespace winrt::AemeathPet::WinUI::implementation
