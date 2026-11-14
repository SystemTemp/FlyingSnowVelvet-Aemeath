#include "TestSupport.h"

#include "Aemeath/Core/Network/Http.h"
#include "Aemeath/Infrastructure/Http/MockHttpClient.h"

#include <string>
#include <vector>

namespace {

using Aemeath::Core::Network::HttpMethod;
using Aemeath::Core::Network::HttpRequest;
using Aemeath::Infrastructure::Http::MockHttpClient;

AE_TEST(Http_MockRoutesByPrefix) {
    MockHttpClient client;
    client.RouteJson("https://api.example.com/posts", 200, R"({"ok":true})");

    HttpRequest request;
    request.url = "https://api.example.com/posts?page=1";
    request.method = HttpMethod::Get;

    const auto response = client.SendAsync(request).get();
    AE_EXPECT_EQ(response.statusCode, 200);
    AE_EXPECT_EQ(response.BodyText(), std::string(R"({"ok":true})"));
}

AE_TEST(Http_MockUnmatchedReturns404) {
    MockHttpClient client;
    HttpRequest request;
    request.url = "https://nope.example.com/x";
    const auto response = client.SendAsync(request).get();
    AE_EXPECT_EQ(response.statusCode, 404);
    AE_EXPECT(!response.error.empty());
    AE_EXPECT(!response.IsSuccess());
}

AE_TEST(Http_MockRecordsRequests) {
    MockHttpClient client;
    client.RouteJson("https://api.example.com/a", 200, "{}");

    HttpRequest first;
    first.url = "https://api.example.com/a";
    first.method = HttpMethod::Post;
    first.headers["x-test"] = "1";
    first.body = { 'h', 'i' };
    (void)client.SendAsync(first).get();

    HttpRequest second;
    second.url = "https://api.example.com/b";
    (void)client.SendAsync(second).get();

    const auto recorded = client.Requests();
    AE_EXPECT_EQ(recorded.size(), static_cast<std::size_t>(2));
    AE_EXPECT_EQ(recorded[0].url, std::string("https://api.example.com/a"));
    AE_EXPECT(recorded[0].method == HttpMethod::Post);
    AE_EXPECT_EQ(recorded[0].headers.at("x-test"), std::string("1"));
    AE_EXPECT_EQ(recorded[0].body.size(), static_cast<std::size_t>(2));
}

AE_TEST(Http_MockStreamingDeliversBodyOnce) {
    MockHttpClient client;
    client.RouteJson("https://api.example.com/sse", 200, "data: {}\n\n");

    int headerStatus = 0;
    std::string collected;

    HttpRequest request;
    request.url = "https://api.example.com/sse";

    const auto code = client
                          .SendStreamingAsync(
                              request,
                              [&](int status, const std::map<std::string, std::string>&) {
                                  headerStatus = status;
                              },
                              [&](std::string_view bytes) {
                                  collected.append(bytes);
                              })
                          .get();

    AE_EXPECT_EQ(code, static_cast<std::int64_t>(0));
    AE_EXPECT_EQ(headerStatus, 200);
    AE_EXPECT_EQ(collected, std::string("data: {}\n\n"));
}

AE_TEST(Http_CustomHandlerSeesRequest) {
    MockHttpClient client;
    client.Route("https://api.example.com/echo", [](const HttpRequest& request) {
        Aemeath::Core::Network::HttpResponse response;
        response.statusCode = 201;
        const std::string text(request.body.begin(), request.body.end());
        response.body.assign(text.begin(), text.end());
        return response;
    });

    HttpRequest request;
    request.url = "https://api.example.com/echo";
    request.method = HttpMethod::Post;
    request.body = { 'p', 'i', 'n', 'g' };

    const auto response = client.SendAsync(request).get();
    AE_EXPECT_EQ(response.statusCode, 201);
    AE_EXPECT_EQ(response.BodyText(), std::string("ping"));
}

} // namespace

