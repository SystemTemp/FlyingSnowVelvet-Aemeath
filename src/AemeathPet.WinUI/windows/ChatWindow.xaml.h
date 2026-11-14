#pragma once

#include "ChatWindow.g.h"
#include "Aemeath/Core/Chat/ChatMessage.h"
#include "Aemeath/Services/IChatService.h"

namespace winrt::AemeathPet::WinUI::implementation {

struct ChatWindow : ChatWindowT<ChatWindow> {
    ChatWindow();
    void SendButton_Click(IInspectable const& sender, Microsoft::UI::Xaml::RoutedEventArgs const& args);

private:
    Aemeath::Core::Chat::ChatSession m_session;
    Aemeath::Services::MockChatService m_chatService;
    void AppendMessage(std::wstring const& text);
};

} // namespace winrt::AemeathPet::WinUI::implementation
