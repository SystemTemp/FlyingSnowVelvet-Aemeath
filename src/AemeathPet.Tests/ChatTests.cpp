#include "TestSupport.h"

#include "Aemeath/Core/Chat/ChatMessage.h"
#include "Aemeath/Infrastructure/Chat/ChatProviderRegistry.h"
#include "Aemeath/Infrastructure/Chat/StreamingChatSession.h"
#include "Aemeath/Infrastructure/Http/MockHttpClient.h"
#include "Aemeath/Services/IChatService.h"
#include "Aemeath/Services/IChatProvider.h"

#include <memory>
#include <string>
#include <vector>

namespace {

using Aemeath::Core::Chat::ChatMessage;
using Aemeath::Core::Chat::ChatRole;
using Aemeath::Infrastructure::Http::MockHttpClient;
using Aemeath::Services::ChatProviderConfig;
using Aemeath::Services::ChatProviderKind;

ChatMessage MakeUser(const std::string& text) {
    ChatMessage message;
    message.id = "u1";
    message.role = ChatRole::User;
    message.content = text;
    return message;
}

AE_TEST(Chat_MockServiceEchoes) {
    Aemeath::Services::MockChatService service;
    const auto reply = service.SendAsync({}, MakeUser("\u4f60\u597d")).get();
    AE_EXPECT(reply.content.find("\u4f60\u597d") != std::string::npos);
    AE_EXPECT(reply.role == ChatRole::Assistant);
}

AE_TEST(Chat_ProviderKindParsing) {
    using Aemeath::Services::ParseChatProviderKind;
    using Aemeath::Services::ToString;

    AE_EXPECT(ParseChatProviderKind("openai") == ChatProviderKind::OpenAICompatible);
    AE_EXPECT(ParseChatProviderKind("ollama") == ChatProviderKind::Ollama);
    AE_EXPECT(ParseChatProviderKind("quick-welfare") == ChatProviderKind::QuickWelfare);
    AE_EXPECT(ParseChatProviderKind("backup-welfare") == ChatProviderKind::BackupWelfare);
    AE_EXPECT(ParseChatProviderKind("whatever") == ChatProviderKind::Mock);
    AE_EXPECT_EQ(std::string(ToString(ChatProviderKind::Ollama)), std::string("ollama"));
}

AE_TEST(Chat_StreamingParsesSse) {
    auto http = std::make_shared<MockHttpClient>();
    http->RouteJson(
        "https://api.example.com",
        200,
        "data: {\"choices\":[{\"delta\":{\"content\":\"\u4f60\u597d\"}}]}\n\n"
        "data: {\"choices\":[{\"delta\":{\"content\":\"\uff0c\u4e16\u754c\"}}]}\n\n"
        "data: [DONE]\n\n");

    ChatProviderConfig config;
    config.kind = ChatProviderKind::OpenAICompatible;
    config.endpoint = "https://api.example.com/v1";
    config.model = "test-model";
    config.stream = true;

    Aemeath::Infrastructure::Chat::StreamingChatSession session(http, config);

    std::string streamed;
    const auto reply = session
                           .SendStreamingAsync(
                               {},
                               MakeUser("hi"),
                               [&](const Aemeath::Services::ChatStreamChunk& chunk) {
                                   if (!chunk.delta.empty()) {
                                       streamed += chunk.delta;
                                   }
                               })
                           .get();

    AE_EXPECT_EQ(reply.content, std::string("\u4f60\u597d\uff0c\u4e16\u754c"));
    AE_EXPECT_EQ(streamed, std::string("\u4f60\u597d\uff0c\u4e16\u754c"));
}

AE_TEST(Chat_NonStreamingParsesMessage) {
    auto http = std::make_shared<MockHttpClient>();
    http->RouteJson(
        "https://api.example.com",
        200,
        R"({"choices":[{"message":{"role":"assistant","content":"hello there"}}]})");

    ChatProviderConfig config;
    config.kind = ChatProviderKind::OpenAICompatible;
    config.endpoint = "https://api.example.com/v1";
    config.model = "test-model";
    config.stream = false;

    Aemeath::Infrastructure::Chat::StreamingChatSession session(http, config);
    const auto reply = session.SendAsync({}, MakeUser("hi")).get();
    AE_EXPECT_EQ(reply.content, std::string("hello there"));
}

AE_TEST(Chat_HttpErrorBecomesFriendlyMessage) {
    auto http = std::make_shared<MockHttpClient>();
    // 未注册路由 → Mock 返回 404
    ChatProviderConfig config;
    config.kind = ChatProviderKind::OpenAICompatible;
    config.endpoint = "https://api.example.com/v1";
    config.model = "test-model";

    Aemeath::Infrastructure::Chat::StreamingChatSession session(http, config);
    const auto reply = session.SendAsync({}, MakeUser("hi")).get();

    AE_EXPECT_EQ(reply.id, std::string("error"));
    AE_EXPECT(reply.content.find("\u26a0") != std::string::npos);
}

AE_TEST(Chat_FactoryReturnsMockForMockKind) {
    auto http = std::make_shared<MockHttpClient>();
    ChatProviderConfig config;
    config.kind = ChatProviderKind::Mock;
    const auto service = Aemeath::Services::ChatServiceFactory::Create(config, http);
    AE_EXPECT(service != nullptr);
    const auto reply = service->SendAsync({}, MakeUser("hi")).get();
    AE_EXPECT(!reply.content.empty());
}

AE_TEST(Chat_SystemPromptIsIncludedInRequest) {
    auto http = std::make_shared<MockHttpClient>();
    http->RouteJson("https://api.example.com", 200,
        R"({"choices":[{"message":{"content":"ok"}}]})");

    ChatProviderConfig config;
    config.kind = ChatProviderKind::OpenAICompatible;
    config.endpoint = "https://api.example.com/v1";
    config.model = "m";
    config.stream = false;
    config.systemPrompt = "PERSONA-MARKER";

    Aemeath::Infrastructure::Chat::StreamingChatSession session(http, config);
    (void)session.SendAsync({}, MakeUser("hi")).get();

    const auto requests = http->Requests();
    AE_EXPECT_EQ(requests.size(), static_cast<std::size_t>(1));
    const std::string body(requests[0].body.begin(), requests[0].body.end());
    AE_EXPECT(body.find("PERSONA-MARKER") != std::string::npos);
    AE_EXPECT(body.find("\"model\":\"m\"") != std::string::npos);
}

} // namespace

