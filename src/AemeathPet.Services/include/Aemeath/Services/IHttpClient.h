#pragma once

#include "Aemeath/Core/Network/Http.h"

#include <cstdint>
#include <functional>
#include <future>
#include <map>
#include <string>
#include <string_view>

namespace Aemeath::Services {

class IHttpClient {
public:
    virtual ~IHttpClient() = default;
    virtual std::future<Aemeath::Core::Network::HttpResponse> SendAsync(
        Aemeath::Core::Network::HttpRequest request) = 0;
};

/// 可选能力：能边收边回调的 HTTP 客户端（SSE / NDJSON 流式对话需要）。
/// 实现者应当保证回调在同一个后台线程上串行触发。
class IStreamingHttpClient {
public:
    virtual ~IStreamingHttpClient() = default;

    using HeaderCallback =
        std::function<void(int statusCode, const std::map<std::string, std::string>& headers)>;
    using DataCallback = std::function<void(std::string_view bytes)>;

    /// 返回 0 表示正常结束，其它值为错误码。
    virtual std::future<std::int64_t> SendStreamingAsync(
        Aemeath::Core::Network::HttpRequest request,
        HeaderCallback onHeaders,
        DataCallback onData) = 0;
};

} // namespace Aemeath::Services