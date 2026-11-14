#include "Aemeath/Infrastructure/Http/MockHttpClient.h"

#include <utility>

namespace Aemeath::Infrastructure::Http {

void MockHttpClient::Route(std::string urlPrefix, Handler handler) {
    std::lock_guard lock(m_mutex);
    m_routes.emplace_back(std::move(urlPrefix), std::move(handler));
}

void MockHttpClient::RouteJson(std::string urlPrefix, int status, std::string body) {
    const auto payload = std::make_shared<std::string>(std::move(body));
    Route(
        std::move(urlPrefix),
        [status, payload](const Aemeath::Core::Network::HttpRequest&) {
            Aemeath::Core::Network::HttpResponse response;
            response.statusCode = status;
            response.body.assign(payload->begin(), payload->end());
            response.headers["content-type"] = "application/json";
            return response;
        });
}

std::vector<Aemeath::Core::Network::HttpRequest> MockHttpClient::Requests() const {
    std::lock_guard lock(m_mutex);
    return m_requests;
}

void MockHttpClient::Clear() {
    std::lock_guard lock(m_mutex);
    m_routes.clear();
    m_requests.clear();
}

Aemeath::Core::Network::HttpResponse MockHttpClient::Resolve(
    const Aemeath::Core::Network::HttpRequest& request) {
    std::lock_guard lock(m_mutex);
    m_requests.push_back(request);

    for (const auto& [prefix, handler] : m_routes) {
        if (request.url.rfind(prefix, 0) == 0) {
            return handler(request);
        }
    }

    Aemeath::Core::Network::HttpResponse response;
    response.statusCode = 404;
    response.error = "no mock route for " + request.url;
    return response;
}

std::future<Aemeath::Core::Network::HttpResponse> MockHttpClient::SendAsync(
    Aemeath::Core::Network::HttpRequest request) {
    return std::async(std::launch::async, [this, request = std::move(request)]() mutable {
        return Resolve(request);
    });
}

std::future<std::int64_t> MockHttpClient::SendStreamingAsync(
    Aemeath::Core::Network::HttpRequest request,
    HeaderCallback onHeaders,
    DataCallback onData) {
    return std::async(
        std::launch::async,
        [this, request = std::move(request), onHeaders = std::move(onHeaders), onData = std::move(onData)]() mutable -> std::int64_t {
            const auto response = Resolve(request);
            if (onHeaders) {
                onHeaders(response.statusCode, response.headers);
            }
            if (onData && !response.body.empty()) {
                onData(std::string_view(
                    reinterpret_cast<const char*>(response.body.data()), response.body.size()));
            }
            return response.error.empty() ? 0 : 1;
        });
}

} // namespace Aemeath::Infrastructure::Http