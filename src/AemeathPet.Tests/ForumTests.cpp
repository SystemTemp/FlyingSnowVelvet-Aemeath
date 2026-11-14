#include "TestSupport.h"

#include "Aemeath/Infrastructure/Forum/ForumApiClient.h"
#include "Aemeath/Infrastructure/Http/MockHttpClient.h"
#include "Aemeath/Services/IForumService.h"

#include <memory>
#include <string>

namespace {

using Aemeath::Infrastructure::Forum::ForumApiClient;
using Aemeath::Infrastructure::Http::MockHttpClient;

AE_TEST(Forum_LoginParsesSession) {
    auto http = std::make_shared<MockHttpClient>();
    http->RouteJson(
        "https://forum.test/api/auth/login",
        200,
        R"({"ok":true,"data":{"token":"tok-123","expires_at":1700000000,"token_type":"Bearer","user":{"id":7,"username":"alice","display_name":"Alice"}}})");

    ForumApiClient client(http, "https://forum.test/api");
    const auto session = client.LoginAsync("alice", "secret").get();

    AE_EXPECT_EQ(session.token, std::string("tok-123"));
    AE_EXPECT_EQ(session.user.username, std::string("alice"));
    AE_EXPECT_EQ(session.user.displayName, std::string("Alice"));
    AE_EXPECT_EQ(session.user.id, static_cast<std::int64_t>(7));
}

AE_TEST(Forum_LoginSendsBearerOnNextRequest) {
    auto http = std::make_shared<MockHttpClient>();
    http->RouteJson(
        "https://forum.test/api/auth/login",
        200,
        R"({"ok":true,"data":{"token":"tok-abc","user":{"id":1,"username":"a"}}})");
    http->RouteJson(
        "https://forum.test/api/auth/me",
        200,
        R"({"ok":true,"data":{"id":1,"username":"a","display_name":"A"}})");

    ForumApiClient client(http, "https://forum.test/api");
    (void)client.LoginAsync("a", "b").get();
    const auto me = client.MeAsync().get();

    AE_EXPECT_EQ(me.username, std::string("a"));

    const auto requests = http->Requests();
    AE_EXPECT(requests.size() >= 2);
    const auto& meRequest = requests.back();
    AE_EXPECT_EQ(meRequest.headers.at("Authorization"), std::string("Bearer tok-abc"));
    AE_EXPECT_EQ(meRequest.headers.at("User-Agent"), std::string("FlyingSnowVelvet-Community/1.0"));
}

AE_TEST(Forum_ListPostsParsesPage) {
    auto http = std::make_shared<MockHttpClient>();
    http->RouteJson(
        "https://forum.test/api/posts",
        200,
        R"({"ok":true,"data":{"posts":[{"id":10,"title":"t1","content":"c1","reply_count":2,"like_count":3,"liked_by_me":true,"author":{"id":1,"username":"u"},"tags":["a","b"]}],"page":1,"per_page":20,"total":1,"total_pages":1,"has_more":false,"sort":"new"}})");

    ForumApiClient client(http, "https://forum.test/api");
    Aemeath::Services::ForumPostQuery query;
    query.page = 1;
    query.perPage = 20;
    query.sort = "new";
    query.keyword = "hello";

    const auto page = client.ListPostsAsync(query).get();

    AE_EXPECT_EQ(page.posts.size(), static_cast<std::size_t>(1));
    AE_EXPECT_EQ(page.posts[0].title, std::string("t1"));
    AE_EXPECT_EQ(page.posts[0].replyCount, 2);
    AE_EXPECT_EQ(page.posts[0].likeCount, 3);
    AE_EXPECT(page.posts[0].likedByMe);
    AE_EXPECT_EQ(page.posts[0].author.username, std::string("u"));
    AE_EXPECT_EQ(page.posts[0].tags.size(), static_cast<std::size_t>(2));
    AE_EXPECT_EQ(page.total, 1);
    AE_EXPECT(!page.hasMore);

    // 查询参数应拼进 URL
    const auto requests = http->Requests();
    AE_EXPECT(requests.back().url.find("q=hello") != std::string::npos);
}

AE_TEST(Forum_ApiErrorSurfacesAsException) {
    auto http = std::make_shared<MockHttpClient>();
    http->RouteJson(
        "https://forum.test/api/posts/1",
        200,
        R"({"ok":false,"error":"post not found"})");

    ForumApiClient client(http, "https://forum.test/api");

    bool threw = false;
    std::string message;
    try {
        (void)client.GetPostAsync(1).get();
    } catch (const std::exception& ex) {
        threw = true;
        message = ex.what();
    }

    AE_EXPECT(threw);
    AE_EXPECT(message.find("post not found") != std::string::npos);
}

AE_TEST(Forum_LikeToggleUsesCorrectMethod) {
    auto http = std::make_shared<MockHttpClient>();
    http->RouteJson(
        "https://forum.test/api/posts/5/like",
        200,
        R"({"ok":true,"data":{"liked":true,"like_count":9}})");

    ForumApiClient client(http, "https://forum.test/api");
    const auto result = client.LikePostAsync(5, true).get();

    AE_EXPECT(result.liked);
    AE_EXPECT_EQ(result.likeCount, 9);
    const auto requests = http->Requests();
    AE_EXPECT(requests.back().method == Aemeath::Core::Network::HttpMethod::Post);
}

} // namespace
