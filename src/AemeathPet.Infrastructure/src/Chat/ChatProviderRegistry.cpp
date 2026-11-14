#include "Aemeath/Infrastructure/Chat/ChatProviderRegistry.h"

#include "Aemeath/Infrastructure/Chat/StreamingChatSession.h"

#include "Aemeath/Core/Logging/Logger.h"
#include "Aemeath/Services/IChatService.h"
#include "Aemeath/Services/IChatProvider.h"

#include <memory>
#include <string>
#include <string_view>

namespace Aemeath::Services {

ChatProviderKind ParseChatProviderKind(std::string_view name) noexcept {
    if (name == "openai" || name == "openai-compatible" || name == "manual" || name == "custom") {
        return ChatProviderKind::OpenAICompatible;
    }
    if (name == "ollama") {
        return ChatProviderKind::Ollama;
    }
    if (name == "quick-welfare" || name == "welfare" || name == "kilo" || name == "free") {
        return ChatProviderKind::QuickWelfare;
    }
    if (name == "backup-welfare" || name == "backup" || name == "agnes") {
        return ChatProviderKind::BackupWelfare;
    }
    return ChatProviderKind::Mock;
}

std::string_view ToString(ChatProviderKind kind) noexcept {
    switch (kind) {
    case ChatProviderKind::Mock: return "mock";
    case ChatProviderKind::OpenAICompatible: return "openai";
    case ChatProviderKind::Ollama: return "ollama";
    case ChatProviderKind::QuickWelfare: return "quick-welfare";
    case ChatProviderKind::BackupWelfare: return "backup-welfare";
    }
    return "mock";
}

std::shared_ptr<IChatService> ChatServiceFactory::Create(
    const ChatProviderConfig& config,
    std::shared_ptr<IHttpClient> httpClient) {
    if (config.kind == ChatProviderKind::Mock || httpClient == nullptr) {
        return std::make_shared<MockChatService>();
    }

    ChatProviderConfig effective = config;

    switch (effective.kind) {
    case ChatProviderKind::Ollama:
        if (effective.endpoint.empty()) {
            effective.endpoint = "http://localhost:11434";
        }
        if (effective.model.empty()) {
            effective.model = "llama3";
        }
        break;
    case ChatProviderKind::QuickWelfare:
    case ChatProviderKind::BackupWelfare:
        // Kilo 网关是 OpenAI 兼容端点，且对非空 Bearer 直接 401，因此默认不带 key。
        if (effective.endpoint.empty()) {
            effective.endpoint = "https://ai-gateway.kilo.ai/api/v1";
        }
        if (effective.kind == ChatProviderKind::QuickWelfare) {
            effective.apiKey.clear();
        }
        if (effective.model.empty()) {
            effective.model = effective.kind == ChatProviderKind::QuickWelfare
                ? "glm-4.5-air"
                : "agnes-2.0-flash";
        }
        break;
    case ChatProviderKind::OpenAICompatible:
    default:
        if (effective.endpoint.empty()) {
            Aemeath::Core::Logging::Logger::Instance().Warn(
                "chat", "\u624b\u52a8 API \u6765\u6e90\u672a\u914d\u7f6e endpoint\uff0c\u8bf7\u5728\u8bbe\u7f6e\u91cc\u586b\u5199");
        }
        break;
    }

    return std::make_shared<Aemeath::Infrastructure::Chat::StreamingChatSession>(
        std::move(httpClient), std::move(effective));
}

} // namespace Aemeath::Services

namespace Aemeath::Infrastructure::Chat {

std::shared_ptr<Aemeath::Services::IChatService> CreateChatService(
    const Aemeath::Core::Config::ChatSettings& settings,
    std::shared_ptr<Aemeath::Services::IHttpClient> httpClient) {
    Aemeath::Services::ChatProviderConfig config;
    config.kind = Aemeath::Services::ParseChatProviderKind(settings.provider);
    config.endpoint = settings.endpoint;
    config.model = settings.model;
    config.temperature = settings.temperature;
    config.stream = settings.stream;
    config.timeoutSeconds = settings.timeoutSeconds;
    return Aemeath::Services::ChatServiceFactory::Create(config, std::move(httpClient));
}

} // namespace Aemeath::Infrastructure::Chat