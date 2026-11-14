#pragma once

#include "Aemeath/Core/Chat/ChatMessage.h"
#include <future>
#include <vector>

namespace Aemeath::Services {

class IChatService {
public:
    virtual ~IChatService() = default;

    virtual std::future<Aemeath::Core::Chat::ChatMessage> SendAsync(
        std::vector<Aemeath::Core::Chat::ChatMessage> history,
        Aemeath::Core::Chat::ChatMessage userMessage) = 0;
};

class MockChatService final : public IChatService {
public:
    std::future<Aemeath::Core::Chat::ChatMessage> SendAsync(
        std::vector<Aemeath::Core::Chat::ChatMessage> history,
        Aemeath::Core::Chat::ChatMessage userMessage) override {
        (void)history;
        return std::async(std::launch::async, [message = std::move(userMessage)]() mutable {
            Aemeath::Core::Chat::ChatMessage reply;
            reply.id = "mock-reply";
            reply.role = Aemeath::Core::Chat::ChatRole::Assistant;
            reply.content = "收到：" + message.content + "。我是 C++ 版飞行雪绒桌宠。";
            return reply;
        });
    }
};

} // namespace Aemeath::Services
