#pragma once

#include "Aemeath/Services/IHttpClient.h"

#include <string>

namespace Aemeath::Infrastructure::Http {

/// 基于 WinHTTP 的实现：每个请求独立 session，线程安全，无需外部依赖。
class WinHttpClient final : public Aemeath::Services::IHttpClient,
                            public Aemeath::Services::IStreamingHttpClient {
public:
    explicit WinHttpClient(std::string userAgent = "FlyingSnowVelvet-Aemeath-Cpp/1.0");

    std::future<Aemeath::Core::Network::HttpResponse> SendAsync(
        Aemeath::Core::Network::HttpRequest request) override;

    std::future<std::int64_t> SendStreamingAsync(
        Aemeath::Core::Network::HttpRequest request,
        HeaderCallback onHeaders,
        DataCallback onData) override;

private:
    std::string m_userAgent;
};

} // namespace Aemeath::Infrastructure::Http