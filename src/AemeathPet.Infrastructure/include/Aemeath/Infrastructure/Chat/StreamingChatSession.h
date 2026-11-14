#pragma once

#include "Aemeath/Services/IChatProvider.h"
#include "Aemeath/Services/IChatService.h"
#include "Aemeath/Services/IHttpClient.h"
#include "Aemeath/Services/IStreamingChatService.h"

#include <memory>
#include <string>

namespace Aemeath::Infrastructure::Chat {

/// OpenAI 兼容 / Ollama 的统一客户端：支持 SSE 与 NDJSON 两种流式格式。
class StreamingChatSession final : public Aemeath::Services::IChatService,
                                   public Aemeath::Services::IStreamingChatService {
public:
    StreamingChatSession(
        std::shared_ptr<Aemeath::Services::IHttpClient> httpClient,
        Aemeath::Services::ChatProviderConfig config);

    std::future<Aemeath::Core::Chat::ChatMessage> SendAsync(
        std::vector<Aemeath::Core::Chat::ChatMessage> history,
        Aemeath::Core::Chat::ChatMessage userMessage) override;

    std::future<Aemeath::Core::Chat::ChatMessage> SendStreamingAsync(
        std::vector<Aemeath::Core::Chat::ChatMessage> history,
        Aemeath::Core::Chat::ChatMessage userMessage,
        Aemeath::Services::ChatStreamCallback onChunk) override;

private:
    Aemeath::Core::Network::HttpRequest BuildRequest(
        const std::vector<Aemeath::Core::Chat::ChatMessage>& history,
        const Aemeath::Core::Chat::ChatMessage& userMessage) const;

    std::shared_ptr<Aemeath::Services::IHttpClient> m_httpClient;
    Aemeath::Services::ChatProviderConfig m_config;
    bool m_ollama { false };
};

} // namespace Aemeath::Infrastructure::Chat