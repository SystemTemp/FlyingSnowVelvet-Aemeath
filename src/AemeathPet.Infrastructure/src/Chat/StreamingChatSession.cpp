#include "Aemeath/Infrastructure/Chat/StreamingChatSession.h"

#include "Aemeath/Core/Logging/Logger.h"
#include "Aemeath/Infrastructure/Json/SimpleJson.h"

#include <algorithm>
#include <chrono>
#include <string>
#include <utility>
#include <vector>

namespace Aemeath::Infrastructure::Chat {

namespace J = Aemeath::Infrastructure::Json;
using Aemeath::Core::Chat::ChatMessage;
using Aemeath::Core::Chat::ChatRole;
using Aemeath::Core::Network::HttpMethod;
using Aemeath::Core::Network::HttpRequest;

namespace {

const char* RoleName(ChatRole role) {
    switch (role) {
    case ChatRole::System: return "system";
    case ChatRole::Assistant: return "assistant";
    case ChatRole::User: break;
    }
    return "user";
}

bool EndsWith(std::string_view text, std::string_view suffix) {
    return text.size() >= suffix.size()
        && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string TrimRightSlashes(std::string value) {
    while (!value.empty() && (value.back() == '/' || value.back() == ' ')) {
        value.pop_back();
    }
    return value;
}

std::string ChatCompletionsUrl(const std::string& endpoint) {
    if (endpoint.empty()) {
        return {};
    }
    auto base = TrimRightSlashes(endpoint);
    if (EndsWith(base, "/chat/completions")) {
        return base;
    }
    if (EndsWith(base, "/v1")) {
        return base + "/chat/completions";
    }
    return base + "/v1/chat/completions";
}

std::string OllamaChatUrl(const std::string& endpoint) {
    auto base = endpoint.empty() ? std::string("http://localhost:11434") : TrimRightSlashes(endpoint);
    if (EndsWith(base, "/api/chat")) {
        return base;
    }
    return base + "/api/chat";
}

std::string MakeMessageId() {
    const auto now = std::chrono::steady_clock::now().time_since_epoch();
    return "chat-" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
}

ChatMessage MakeErrorReply(const std::string& reason) {
    ChatMessage message;
    message.id = "error";
    message.role = ChatRole::Assistant;
    message.content = "\u26a0 " + reason;
    return message;
}

} // namespace

StreamingChatSession::StreamingChatSession(
    std::shared_ptr<Aemeath::Services::IHttpClient> httpClient,
    Aemeath::Services::ChatProviderConfig config)
    : m_httpClient(std::move(httpClient)),
      m_config(std::move(config)) {
    m_ollama = m_config.kind == Aemeath::Services::ChatProviderKind::Ollama;
    if (m_config.timeoutSeconds <= 0) {
        m_config.timeoutSeconds = 60;
    }
    if (m_config.model.empty()) {
        m_config.model = m_ollama ? "llama3" : "gpt-4o-mini";
    }
}

HttpRequest StreamingChatSession::BuildRequest(
    const std::vector<ChatMessage>& history,
    const ChatMessage& userMessage) const {
    HttpRequest request;
    request.method = HttpMethod::Post;
    request.timeoutSeconds = m_config.timeoutSeconds;
    request.headers["Content-Type"] = "application/json";
    request.headers["Accept"] = m_ollama ? "application/x-ndjson" : "text/event-stream";
    if (!m_config.apiKey.empty()) {
        request.headers["Authorization"] = "Bearer " + m_config.apiKey;
    }

    J::Value::Array messages;
    if (!m_config.systemPrompt.empty()) {
        messages.emplace_back(J::Value::Object {
            { "role", "system" },
            { "content", m_config.systemPrompt },
        });
    }
    for (const auto& message : history) {
        messages.emplace_back(J::Value::Object {
            { "role", RoleName(message.role) },
            { "content", message.content },
        });
    }
    messages.emplace_back(J::Value::Object {
        { "role", RoleName(userMessage.role) },
        { "content", userMessage.content },
    });

    if (m_ollama) {
        request.url = OllamaChatUrl(m_config.endpoint);
        J::Value::Object body {
            { "model", m_config.model },
            { "messages", messages },
            { "stream", m_config.stream },
        };
        body["options"] = J::Value::Object { { "temperature", m_config.temperature } };
        const auto payload = J::Stringify(J::Value(std::move(body)));
        request.body.assign(payload.begin(), payload.end());
        return request;
    }

    request.url = ChatCompletionsUrl(m_config.endpoint);
    J::Value::Object body {
        { "model", m_config.model },
        { "messages", messages },
        { "stream", m_config.stream },
    };
    body["temperature"] = m_config.temperature;
    if (m_config.maxTokens > 0) {
        body["max_tokens"] = m_config.maxTokens;
    }
    const auto payload = J::Stringify(J::Value(std::move(body)));
    request.body.assign(payload.begin(), payload.end());
    return request;
}

std::future<ChatMessage> StreamingChatSession::SendAsync(
    std::vector<ChatMessage> history,
    ChatMessage userMessage) {
    return SendStreamingAsync(std::move(history), std::move(userMessage), nullptr);
}

std::future<ChatMessage> StreamingChatSession::SendStreamingAsync(
    std::vector<ChatMessage> history,
    ChatMessage userMessage,
    Aemeath::Services::ChatStreamCallback onChunk) {
    auto localConfig = m_config;

    return std::async(
        std::launch::async,
        [this,
         history = std::move(history),
         userMessage = std::move(userMessage),
         onChunk = std::move(onChunk),
         localConfig]() mutable -> ChatMessage {
            ChatMessage reply;
            reply.id = MakeMessageId();
            reply.role = ChatRole::Assistant;

            if (m_httpClient == nullptr) {
                return MakeErrorReply("\u6ca1\u6709\u53ef\u7528\u7684 HTTP \u5ba2\u6237\u7aef");
            }

            const auto request = BuildRequest(history, userMessage);
            if (request.url.empty()) {
                return MakeErrorReply("\u672a\u914d\u7f6e API \u5730\u5740\uff0c\u8bf7\u5728\u8bbe\u7f6e\u91cc\u586b\u5199");
            }

            std::string text;
            bool finished = false;
            std::string failure;

            auto appendDelta = [&](std::string piece) {
                if (piece.empty()) {
                    return;
                }
                text += piece;
                if (onChunk) {
                    onChunk(Aemeath::Services::ChatStreamChunk { piece, false, {} });
                }
            };

            std::string buffer;
            auto processLine = [&](std::string line) {
                while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) {
                    line.pop_back();
                }
                if (line.empty()) {
                    return;
                }
                if (line.front() == ':') {
                    return; // SSE 注释行
                }

                if (m_ollama) {
                    try {
                        const auto value = J::Parse(line);
                        if (const auto* message = value.Find("message"); message != nullptr) {
                            appendDelta(J::String(*message, "content"));
                        }
                        if (const auto* response = value.Find("response");
                            response != nullptr && response->IsString()) {
                            appendDelta(response->AsString());
                        }
                        if (J::Bool(value, "done", false)) {
                            finished = true;
                        }
                    } catch (const std::exception&) {
                        // 单行解析失败不致命：跳过。
                    }
                    return;
                }

                if (line.rfind("data:", 0) != 0) {
                    return;
                }
                std::string payload = line.substr(5);
                while (!payload.empty() && payload.front() == ' ') {
                    payload.erase(payload.begin());
                }
                if (payload == "[DONE]") {
                    finished = true;
                    return;
                }
                try {
                    const auto value = J::Parse(payload);
                    const auto* choices = value.Find("choices");
                    if (choices == nullptr || !choices->IsArray() || choices->AsArray().empty()) {
                        return;
                    }
                    const auto& first = choices->AsArray().front();
                    if (const auto* delta = first.Find("delta"); delta != nullptr) {
                        if (const auto* content = delta->Find("content");
                            content != nullptr && content->IsString()) {
                            appendDelta(content->AsString());
                        }
                    } else if (const auto* message = first.Find("message"); message != nullptr) {
                        appendDelta(J::String(*message, "content"));
                    }
                } catch (const std::exception&) {
                    // 可能是被截断的半个 JSON，忽略。
                }
            };

            auto* streaming = dynamic_cast<Aemeath::Services::IStreamingHttpClient*>(m_httpClient.get());
            if (streaming != nullptr && localConfig.stream) {
                int status = 0;
                const auto code = streaming->SendStreamingAsync(
                    request,
                    [&status, &failure](int statusCode, const std::map<std::string, std::string>&) {
                        status = statusCode;
                        if (statusCode != 200) {
                            failure = "\u670d\u52a1\u8fd4\u56de HTTP " + std::to_string(statusCode);
                        }
                    },
                    [&](std::string_view bytes) {
                        buffer.append(bytes);
                        std::size_t position = 0;
                        while ((position = buffer.find('\n')) != std::string::npos) {
                            std::string line = buffer.substr(0, position);
                            buffer.erase(0, position + 1);
                            processLine(std::move(line));
                        }
                    })
                                   .get();

                if (!buffer.empty()) {
                    processLine(buffer);
                    buffer.clear();
                }
                if (code != 0 && failure.empty()) {
                    failure = "HTTP \u8bf7\u6c42\u5931\u8d25\uff08\u9519\u8bef\u7801 "
                        + std::to_string(code) + "\uff09";
                }
            } else {
                const auto response = m_httpClient->SendAsync(request).get();
                if (!response.IsSuccess()) {
                    failure = response.error.empty()
                        ? ("HTTP " + std::to_string(response.statusCode))
                        : response.error;
                } else {
                    try {
                        const auto value = J::Parse(response.BodyText());
                        if (const auto* choices = value.Find("choices");
                            choices != nullptr && choices->IsArray() && !choices->AsArray().empty()) {
                            const auto& first = choices->AsArray().front();
                            if (const auto* message = first.Find("message"); message != nullptr) {
                                appendDelta(J::String(*message, "content"));
                            }
                            finished = true;
                        } else if (const auto* message = value.Find("message"); message != nullptr) {
                            appendDelta(J::String(*message, "content"));
                            finished = true;
                        } else if (const auto* response2 = value.Find("response");
                                   response2 != nullptr && response2->IsString()) {
                            appendDelta(response2->AsString());
                            finished = true;
                        } else if (const auto* error = value.Find("error"); error != nullptr) {
                            failure = J::String(value, "error", "unknown error");
                        }
                    } catch (const std::exception& ex) {
                        failure = std::string("\u54cd\u5e94\u89e3\u6790\u5931\u8d25\uff1a") + ex.what();
                    }
                }
            }

            if (!failure.empty() && text.empty()) {
                reply = MakeErrorReply(failure);
                if (onChunk) {
                    onChunk(Aemeath::Services::ChatStreamChunk { {}, true, failure });
                }
                return reply;
            }

            reply.content = text.empty() ? "\uff08\u6a21\u578b\u8fd4\u56de\u7a7a\u5185\u5bb9\uff09" : text;
            if (onChunk) {
                onChunk(Aemeath::Services::ChatStreamChunk {
                    {},
                    true,
                    failure.empty() ? std::string {} : failure,
                });
            }
            Aemeath::Core::Logging::Logger::Instance().Debug(
                "chat",
                "\u56de\u590d\u5b8c\u6210\uff0c" + std::to_string(text.size()) + " \u5b57\u8282\uff0c"
                    + (finished ? "\u5df2\u6b63\u5e38\u7ed3\u675f" : "\u6d41\u88ab\u622a\u65ad"));
            return reply;
        });
}

} // namespace Aemeath::Infrastructure::Chat