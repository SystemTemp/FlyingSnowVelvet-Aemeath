#include "Aemeath/Infrastructure/Http/WinHttpClient.h"

#include "Aemeath/Core/Logging/Logger.h"

#include <windows.h>
#include <winhttp.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>
#include <vector>

namespace Aemeath::Infrastructure::Http {

namespace {

using Aemeath::Core::Network::HttpMethod;
using Aemeath::Core::Network::HttpRequest;
using Aemeath::Core::Network::HttpResponse;

std::wstring Utf8ToWide(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int size = ::MultiByteToWideChar(
        CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    if (size <= 0) {
        return {};
    }
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    ::MultiByteToWideChar(
        CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), result.data(), size);
    return result;
}

std::string WideToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int size = ::WideCharToMultiByte(
        CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return {};
    }
    std::string result(static_cast<std::size_t>(size), '\0');
    ::WideCharToMultiByte(
        CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), result.data(), size, nullptr, nullptr);
    return result;
}

std::string Trim(std::string value) {
    auto notSpace = [](unsigned char c) { return std::isspace(c) == 0; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), notSpace));
    value.erase(std::find_if(value.rbegin(), value.rend(), notSpace).base(), value.end());
    return value;
}

std::string ToLowerAscii(std::string value) {
    for (auto& c : value) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return value;
}

const wchar_t* MethodName(HttpMethod method) {
    switch (method) {
    case HttpMethod::Get: return L"GET";
    case HttpMethod::Post: return L"POST";
    case HttpMethod::Put: return L"PUT";
    case HttpMethod::Patch: return L"PATCH";
    case HttpMethod::Delete: return L"DELETE";
    }
    return L"GET";
}

class HandleGuard final {
public:
    HandleGuard() = default;
    explicit HandleGuard(HINTERNET handle) : m_handle(handle) {}
    ~HandleGuard() { Reset(); }

    HandleGuard(const HandleGuard&) = delete;
    HandleGuard& operator=(const HandleGuard&) = delete;

    HandleGuard(HandleGuard&& other) noexcept : m_handle(other.m_handle) { other.m_handle = nullptr; }
    HandleGuard& operator=(HandleGuard&& other) noexcept {
        if (this != &other) {
            Reset();
            m_handle = other.m_handle;
            other.m_handle = nullptr;
        }
        return *this;
    }

    void Reset(HINTERNET handle = nullptr) {
        if (m_handle != nullptr) {
            ::WinHttpCloseHandle(m_handle);
        }
        m_handle = handle;
    }

    [[nodiscard]] HINTERNET Get() const noexcept { return m_handle; }
    [[nodiscard]] bool Valid() const noexcept { return m_handle != nullptr; }

private:
    HINTERNET m_handle { nullptr };
};

struct UrlParts {
    std::wstring host;
    std::wstring path;
    std::wstring extra; // ?query
    INTERNET_PORT port { 0 };
    bool secure { false };
};

bool CrackUrl(const std::wstring& url, UrlParts* parts, std::string* error) {
    URL_COMPONENTS components {};
    components.dwStructSize = sizeof(components);
    components.dwSchemeLength = static_cast<DWORD>(-1);
    components.dwHostNameLength = static_cast<DWORD>(-1);
    components.dwUrlPathLength = static_cast<DWORD>(-1);
    components.dwExtraInfoLength = static_cast<DWORD>(-1);

    if (::WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.size()), 0, &components) == FALSE) {
        *error = "URL \u89e3\u6790\u5931\u8d25\uff08" + std::to_string(::GetLastError()) + "\uff09";
        return false;
    }

    parts->host.assign(components.lpszHostName, components.dwHostNameLength);
    parts->path.assign(components.lpszUrlPath, components.dwUrlPathLength);
    if (components.lpszExtraInfo != nullptr && components.dwExtraInfoLength > 0) {
        parts->extra.assign(components.lpszExtraInfo, components.dwExtraInfoLength);
    }
    parts->port = components.nPort;
    parts->secure = components.nScheme == INTERNET_SCHEME_HTTPS;

    if (parts->path.empty()) {
        parts->path = L"/";
    }
    return true;
}

int ParseStatus(HINTERNET request) {
    DWORD status = 0;
    DWORD size = sizeof(status);
    if (::WinHttpQueryHeaders(
            request,
            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
            WINHTTP_HEADER_NAME_BY_INDEX,
            &status,
            &size,
            WINHTTP_NO_HEADER_INDEX) == FALSE) {
        return 0;
    }
    return static_cast<int>(status);
}

std::map<std::string, std::string> ParseHeaders(HINTERNET request) {
    std::map<std::string, std::string> headers;

    DWORD size = 0;
    ::WinHttpQueryHeaders(
        request,
        WINHTTP_QUERY_RAW_HEADERS_CRLF,
        WINHTTP_HEADER_NAME_BY_INDEX,
        WINHTTP_NO_OUTPUT_BUFFER,
        &size,
        WINHTTP_NO_HEADER_INDEX);

    if (size == 0) {
        return headers;
    }

    std::wstring raw(size / sizeof(wchar_t) + 1, L'\0');
    if (::WinHttpQueryHeaders(
            request,
            WINHTTP_QUERY_RAW_HEADERS_CRLF,
            WINHTTP_HEADER_NAME_BY_INDEX,
            raw.data(),
            &size,
            WINHTTP_NO_HEADER_INDEX) == FALSE) {
        return headers;
    }
    raw.resize(size / sizeof(wchar_t));

    const auto text = WideToUtf8(raw);
    std::size_t start = 0;
    while (start < text.size()) {
        auto end = text.find("\r\n", start);
        if (end == std::string::npos) {
            end = text.size();
        }
        const auto line = text.substr(start, end - start);
        const auto colon = line.find(':');
        if (colon != std::string::npos) {
            headers[ToLowerAscii(Trim(line.substr(0, colon)))] = Trim(line.substr(colon + 1));
        }
        start = end + 2;
    }
    return headers;
}

/// 建立请求（不含读取）。失败时 error 里写原因。
bool PrepareRequest(
    const HttpRequest& request,
    const std::string& userAgent,
    HandleGuard* session,
    HandleGuard* connect,
    HandleGuard* handle,
    std::string* error) {
    UrlParts parts;
    if (!CrackUrl(Utf8ToWide(request.url), &parts, error)) {
        return false;
    }

    session->Reset(::WinHttpOpen(
        Utf8ToWide(userAgent).c_str(),
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0));
    if (!session->Valid()) {
        *error = "WinHttpOpen \u5931\u8d25\uff08" + std::to_string(::GetLastError()) + "\uff09";
        return false;
    }

    const int timeoutSeconds = std::clamp(request.timeoutSeconds, 1, 600);
    const int timeoutMs = timeoutSeconds * 1000;
    ::WinHttpSetTimeouts(session->Get(), timeoutMs, timeoutMs, timeoutMs, timeoutMs);

    connect->Reset(::WinHttpConnect(session->Get(), parts.host.c_str(), parts.port, 0));
    if (!connect->Valid()) {
        *error = "WinHttpConnect \u5931\u8d25\uff08" + std::to_string(::GetLastError()) + "\uff09";
        return false;
    }

    std::wstring target = parts.path;
    target += parts.extra;

    handle->Reset(::WinHttpOpenRequest(
        connect->Get(),
        MethodName(request.method),
        target.c_str(),
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        parts.secure ? WINHTTP_FLAG_SECURE : 0));
    if (!handle->Valid()) {
        *error = "WinHttpOpenRequest \u5931\u8d25\uff08" + std::to_string(::GetLastError()) + "\uff09";
        return false;
    }

    for (const auto& [name, value] : request.headers) {
        const auto line = Utf8ToWide(name + ": " + value);
        ::WinHttpAddRequestHeaders(
            handle->Get(),
            line.c_str(),
            static_cast<DWORD>(-1),
            WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE);
    }

    const void* body = request.body.empty() ? nullptr : request.body.data();
    const DWORD bodySize = static_cast<DWORD>(request.body.size());
    if (::WinHttpSendRequest(
            handle->Get(),
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0,
            const_cast<void*>(body),
            bodySize,
            bodySize,
            0) == FALSE) {
        *error = "WinHttpSendRequest \u5931\u8d25\uff08" + std::to_string(::GetLastError()) + "\uff09";
        return false;
    }

    if (::WinHttpReceiveResponse(handle->Get(), nullptr) == FALSE) {
        *error = "WinHttpReceiveResponse \u5931\u8d25\uff08" + std::to_string(::GetLastError()) + "\uff09";
        return false;
    }
    return true;
}

HttpResponse DoRequest(const HttpRequest& request, const std::string& userAgent) {
    HttpResponse response;
    HandleGuard session;
    HandleGuard connect;
    HandleGuard handle;

    if (!PrepareRequest(request, userAgent, &session, &connect, &handle, &response.error)) {
        return response;
    }

    response.statusCode = ParseStatus(handle.Get());
    response.headers = ParseHeaders(handle.Get());

    std::vector<std::uint8_t> body;
    std::uint8_t buffer[8192];
    for (;;) {
        DWORD available = 0;
        if (::WinHttpQueryDataAvailable(handle.Get(), &available) == FALSE) {
            response.error = "WinHttpQueryDataAvailable \u5931\u8d25\uff08"
                + std::to_string(::GetLastError()) + "\uff09";
            return response;
        }
        if (available == 0) {
            break;
        }
        const DWORD toRead = std::min<DWORD>(available, static_cast<DWORD>(sizeof(buffer)));
        DWORD read = 0;
        if (::WinHttpReadData(handle.Get(), buffer, toRead, &read) == FALSE) {
            response.error = "WinHttpReadData \u5931\u8d25\uff08"
                + std::to_string(::GetLastError()) + "\uff09";
            return response;
        }
        if (read == 0) {
            break;
        }
        body.insert(body.end(), buffer, buffer + read);
    }

    response.body = std::move(body);
    return response;
}

} // namespace

WinHttpClient::WinHttpClient(std::string userAgent)
    : m_userAgent(std::move(userAgent)) {
    if (m_userAgent.empty()) {
        m_userAgent = "FlyingSnowVelvet-Aemeath-Cpp/1.0";
    }
}

std::future<HttpResponse> WinHttpClient::SendAsync(HttpRequest request) {
    const auto agent = m_userAgent;
    return std::async(std::launch::async, [request = std::move(request), agent]() mutable {
        return DoRequest(request, agent);
    });
}

std::future<std::int64_t> WinHttpClient::SendStreamingAsync(
    HttpRequest request,
    HeaderCallback onHeaders,
    DataCallback onData) {
    const auto agent = m_userAgent;
    return std::async(
        std::launch::async,
        [request = std::move(request), agent, onHeaders = std::move(onHeaders), onData = std::move(onData)]() mutable -> std::int64_t {
            HandleGuard session;
            HandleGuard connect;
            HandleGuard handle;
            std::string error;

            if (!PrepareRequest(request, agent, &session, &connect, &handle, &error)) {
                Aemeath::Core::Logging::Logger::Instance().Warn("http", error);
                return static_cast<std::int64_t>(::GetLastError());
            }

            const int status = ParseStatus(handle.Get());
            if (onHeaders) {
                onHeaders(status, ParseHeaders(handle.Get()));
            }

            std::uint8_t buffer[4096];
            for (;;) {
                DWORD available = 0;
                if (::WinHttpQueryDataAvailable(handle.Get(), &available) == FALSE) {
                    return static_cast<std::int64_t>(::GetLastError());
                }
                if (available == 0) {
                    break;
                }
                const DWORD toRead = std::min<DWORD>(available, static_cast<DWORD>(sizeof(buffer)));
                DWORD read = 0;
                if (::WinHttpReadData(handle.Get(), buffer, toRead, &read) == FALSE) {
                    return static_cast<std::int64_t>(::GetLastError());
                }
                if (read == 0) {
                    break;
                }
                if (onData) {
                    onData(std::string_view(
                        reinterpret_cast<const char*>(buffer), static_cast<std::size_t>(read)));
                }
            }
            return 0;
        });
}

} // namespace Aemeath::Infrastructure::Http