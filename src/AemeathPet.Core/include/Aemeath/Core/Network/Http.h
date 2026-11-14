#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace Aemeath::Core::Network {

enum class HttpMethod {
    Get,
    Post,
    Put,
    Patch,
    Delete
};

struct HttpRequest {
    HttpMethod method { HttpMethod::Get };
    std::string url;
    std::map<std::string, std::string> headers;
    std::vector<std::uint8_t> body;
    int timeoutSeconds { 30 };
};

struct HttpResponse {
    int statusCode { 0 };
    std::map<std::string, std::string> headers;
    std::vector<std::uint8_t> body;
    std::string error;

    [[nodiscard]] bool IsSuccess() const noexcept {
        return statusCode >= 200 && statusCode < 300 && error.empty();
    }

    [[nodiscard]] std::string BodyText() const {
        return std::string(body.begin(), body.end());
    }
};

} // namespace Aemeath::Core::Network
