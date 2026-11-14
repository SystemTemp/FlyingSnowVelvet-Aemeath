#pragma once

#include "Aemeath/Core/Chat/ChatMessage.h"

#include <functional>
#include <future>
#include <string>
#include <vector>

namespace Aemeath::Services {

/// 流式增量：delta 为本次新增文本；done 为 true 表示流结束；notice 用于承载
/// "换模型""回退纯文本"等非正文提示。
struct ChatStreamChunk {
    std::string delta;
    bool done { false };
    std::string notice;
};

using ChatStreamCallback = std::function<void(const ChatStreamChunk&)>;

class IStreamingChatService {
public:
    virtual ~IStreamingChatService() = default;

    /// 流式对话。返回完整回复（与逐块回调内容一致），回调可为空。
    virtual std::future<Aemeath::Core::Chat::ChatMessage> SendStreamingAsync(
        std::vector<Aemeath::Core::Chat::ChatMessage> history,
        Aemeath::Core::Chat::ChatMessage userMessage,
        ChatStreamCallback onChunk) = 0;
};

} // namespace Aemeath::Services