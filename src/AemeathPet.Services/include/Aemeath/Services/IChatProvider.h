#pragma once

#include "Aemeath/Services/IChatService.h"
#include "Aemeath/Services/IHttpClient.h"

#include <memory>
#include <string>
#include <string_view>

namespace Aemeath::Services {

/// 对话来源。与原版一致：只使用用户选中的来源，不跨来源自动回退。
enum class ChatProviderKind {
    Mock,
    OpenAICompatible,
    Ollama,
    QuickWelfare,
    BackupWelfare
};

struct ChatProviderConfig {
    ChatProviderKind kind { ChatProviderKind::Mock };
    std::string endpoint;
    std::string apiKey;
    std::string model;
    std::string systemPrompt;
    double temperature { 0.7 };
    int maxTokens { 0 };
    bool stream { true };
    int timeoutSeconds { 60 };
};

[[nodiscard]] ChatProviderKind ParseChatProviderKind(std::string_view name) noexcept;
[[nodiscard]] std::string_view ToString(ChatProviderKind kind) noexcept;

/// 按配置构造具体对话服务；httpClient 可为空（Mock 来源不需要网络）。
class ChatServiceFactory final {
public:
    static std::shared_ptr<IChatService> Create(
        const ChatProviderConfig& config,
        std::shared_ptr<IHttpClient> httpClient);
};

} // namespace Aemeath::Services