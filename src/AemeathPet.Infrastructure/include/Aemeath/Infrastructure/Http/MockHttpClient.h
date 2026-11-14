#pragma once

#include "Aemeath/Services/IHttpClient.h"

#include <functional>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace Aemeath::Infrastructure::Http {

/// 可编程的假 HTTP 客户端：用于离线测试与 Mock 模式。
class MockHttpClient final : public Aemeath::Services::IHttpClient,
                             public Aemeath::Services::IStreamingHttpClient {
public:
    using Handler = std::function<Aemeath::Core::Network::HttpResponse(
        const Aemeath::Core::Network::HttpRequest&)>;

    /// 按注册顺序做前缀匹配。
    void Route(std::string urlPrefix, Handler handler);
    void RouteJson(std::string urlPrefix, int status, std::string body);

    [[nodiscard]] std::vector<Aemeath::Core::Network::HttpRequest> Requests() const;
    void Clear();

    std::future<Aemeath::Core::Network::HttpResponse> SendAsync(
        Aemeath::Core::Network::HttpRequest request) override;

    std::future<std::int64_t> SendStreamingAsync(
        Aemeath::Core::Network::HttpRequest request,
        HeaderCallback onHeaders,
        DataCallback onData) override;

private:
    Aemeath::Core::Network::HttpResponse Resolve(const Aemeath::Core::Network::HttpRequest& request);

    mutable std::mutex m_mutex;
    std::vector<std::pair<std::string, Handler>> m_routes;
    std::vector<Aemeath::Core::Network::HttpRequest> m_requests;
};

} // namespace Aemeath::Infrastructure::Http