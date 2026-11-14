#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace Aemeath::Core::Chat {

enum class ChatRole {
    System,
    User,
    Assistant
};

struct ChatMessage {
    std::string id;
    ChatRole role { ChatRole::User };
    std::string content;
    std::chrono::system_clock::time_point createdAt { std::chrono::system_clock::now() };
};

class ChatSession final {
public:
    void Add(ChatMessage message) { m_messages.emplace_back(std::move(message)); }
    void Clear() { m_messages.clear(); }
    [[nodiscard]] const std::vector<ChatMessage>& Messages() const noexcept { return m_messages; }

private:
    std::vector<ChatMessage> m_messages;
};

} // namespace Aemeath::Core::Chat
