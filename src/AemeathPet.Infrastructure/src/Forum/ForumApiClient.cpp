#include "Aemeath/Infrastructure/Forum/ForumApiClient.h"

#include "Aemeath/Core/Logging/Logger.h"
#include "Aemeath/Infrastructure/Json/SimpleJson.h"

#include <stdexcept>
#include <utility>

namespace Aemeath::Infrastructure::Forum {

namespace J = Aemeath::Infrastructure::Json;
using Aemeath::Core::Forum::ForumImage;
using Aemeath::Core::Forum::ForumLikeResult;
using Aemeath::Core::Forum::ForumPost;
using Aemeath::Core::Forum::ForumPostPage;
using Aemeath::Core::Forum::ForumReply;
using Aemeath::Core::Forum::ForumReplyPage;
using Aemeath::Core::Forum::ForumSession;
using Aemeath::Core::Forum::ForumTag;
using Aemeath::Core::Forum::ForumUser;
using Aemeath::Core::Network::HttpMethod;
using Aemeath::Core::Network::HttpRequest;

namespace {

/// 取出响应里的 data 字段。ok=false 时抛出带服务端错误的异常。
/// 注意：调用方必须让 root 的生命周期覆盖返回引用。
const J::Value& UnwrapData(const J::Value& root) {
    if (const auto* ok = root.Find("ok"); ok != nullptr && ok->IsBool() && !ok->AsBool()) {
        throw std::runtime_error("forum API error: " + J::String(root, "error", "unknown"));
    }
    if (const auto* data = root.Find("data"); data != nullptr) {
        return *data;
    }
    return root;
}

ForumUser ParseUser(const J::Value& value) {
    ForumUser user;
    user.id = J::Int(value, "id");
    user.username = J::String(value, "username");
    user.displayName = J::String(value, "display_name");
    user.bio = J::String(value, "bio");
    user.postCount = static_cast<int>(J::Int(value, "post_count"));
    user.replyCount = static_cast<int>(J::Int(value, "reply_count"));
    user.likesGiven = static_cast<int>(J::Int(value, "likes_given"));
    user.createdAt = J::Int(value, "created_at");
    return user;
}

ForumImage ParseImage(const J::Value& value) {
    ForumImage image;
    image.id = J::String(value, "id");
    image.url = J::String(value, "url");
    image.mime = J::String(value, "mime");
    image.width = static_cast<int>(J::Int(value, "width"));
    image.height = static_cast<int>(J::Int(value, "height"));
    image.size = J::Int(value, "size");
    image.createdAt = J::Int(value, "created_at");
    return image;
}

template <typename T, typename Fn>
std::vector<T> ParseArray(const J::Value& value, std::string_view key, Fn convert) {
    std::vector<T> result;
    const auto* array = value.Find(key);
    if (array == nullptr || !array->IsArray()) {
        return result;
    }
    for (const auto& item : array->AsArray()) {
        result.push_back(convert(item));
    }
    return result;
}

ForumPost ParsePost(const J::Value& value) {
    ForumPost post;
    post.id = J::Int(value, "id");
    post.title = J::String(value, "title");
    post.content = J::String(value, "content");
    post.excerpt = J::String(value, "excerpt");
    post.replyCount = static_cast<int>(J::Int(value, "reply_count"));
    post.likeCount = static_cast<int>(J::Int(value, "like_count"));
    post.viewCount = static_cast<int>(J::Int(value, "view_count"));
    post.likedByMe = J::Bool(value, "liked_by_me");
    post.createdAt = J::Int(value, "created_at");
    post.updatedAt = J::Int(value, "updated_at");
    post.lastReplyAt = J::Int(value, "last_reply_at");

    if (const auto* author = value.Find("author"); author != nullptr) {
        post.author = ParseUser(*author);
    }
    if (const auto* tags = value.Find("tags"); tags != nullptr && tags->IsArray()) {
        for (const auto& tag : tags->AsArray()) {
            if (tag.IsString()) {
                post.tags.push_back(tag.AsString());
            }
        }
    }
    post.images = ParseArray<ForumImage>(value, "images", ParseImage);
    post.imageCount = static_cast<int>(post.images.size());
    return post;
}

ForumReply ParseReply(const J::Value& value) {
    ForumReply reply;
    reply.id = J::Int(value, "id");
    reply.postId = J::Int(value, "post_id");
    if (const auto* parent = value.Find("parent_id");
        parent != nullptr && parent->IsNumber()) {
        reply.parentId = static_cast<std::int64_t>(parent->AsNumber());
    }
    reply.content = J::String(value, "content");
    reply.likeCount = static_cast<int>(J::Int(value, "like_count"));
    reply.likedByMe = J::Bool(value, "liked_by_me");
    reply.createdAt = J::Int(value, "created_at");
    reply.updatedAt = J::Int(value, "updated_at");
    reply.postTitle = J::String(value, "post_title");

    if (const auto* author = value.Find("author"); author != nullptr) {
        reply.author = ParseUser(*author);
    }
    reply.images = ParseArray<ForumImage>(value, "images", ParseImage);
    reply.imageCount = static_cast<int>(reply.images.size());
    return reply;
}

} // namespace

ForumApiClient::ForumApiClient(
    std::shared_ptr<Aemeath::Services::IHttpClient> httpClient,
    std::string baseUrl)
    : m_httpClient(std::move(httpClient)),
      m_baseUrl(std::move(baseUrl)) {
    if (!m_httpClient) {
        throw std::invalid_argument("ForumApiClient 需要 IHttpClient");
    }
    if (m_baseUrl.empty()) {
        m_baseUrl = "https://forum.fxxr.store/api";
    }
    while (!m_baseUrl.empty() && m_baseUrl.back() == '/') {
        m_baseUrl.pop_back();
    }
}

void ForumApiClient::SetBearerToken(std::string token) {
    std::lock_guard lock(m_tokenMutex);
    m_bearerToken = std::move(token);
}

HttpRequest ForumApiClient::JsonRequest(
    HttpMethod method,
    std::string path,
    std::string body) const {
    HttpRequest request;
    request.method = method;
    request.url = m_baseUrl + std::move(path);
    request.headers = {
        { "User-Agent", "FlyingSnowVelvet-Community/1.0" },
        { "Accept", "application/json" },
        { "Content-Type", "application/json" },
    };
    request.body.assign(body.begin(), body.end());

    {
        std::lock_guard lock(m_tokenMutex);
        if (!m_bearerToken.empty()) {
            request.headers["Authorization"] = "Bearer " + m_bearerToken;
        }
    }
    return request;
}

std::string ForumApiClient::WithQuery(
    std::string path,
    const std::vector<std::pair<std::string, std::string>>& query) const {
    char separator = '?';
    for (const auto& [key, value] : query) {
        if (value.empty()) {
            continue;
        }
        path += separator;
        path += key;
        path += '=';
        path += value;
        separator = '&';
    }
    return path;
}

std::future<ForumSession> ForumApiClient::LoginAsync(std::string username, std::string password) {
    return std::async(
        std::launch::async,
        [this, username = std::move(username), password = std::move(password)] {
            const auto body = J::Stringify(J::Value::Object {
                { "username", username },
                { "password", password },
            });
            const auto response = m_httpClient
                                      ->SendAsync(JsonRequest(HttpMethod::Post, "/auth/login", body))
                                      .get();
            const auto parsed = J::Parse(response.BodyText());
            const auto& data = UnwrapData(parsed);

            ForumSession session;
            session.token = J::String(data, "token");
            session.expiresAt = J::Int(data, "expires_at");
            session.tokenType = J::String(data, "token_type", "Bearer");
            if (const auto* user = data.Find("user"); user != nullptr) {
                session.user = ParseUser(*user);
            }
            SetBearerToken(session.token);
            return session;
        });
}

std::future<ForumSession> ForumApiClient::RegisterAsync(
    std::string username,
    std::string password,
    std::string displayName) {
    return std::async(
        std::launch::async,
        [this,
         username = std::move(username),
         password = std::move(password),
         displayName = std::move(displayName)] {
            const auto body = J::Stringify(J::Value::Object {
                { "username", username },
                { "password", password },
                { "display_name", displayName },
            });
            const auto response = m_httpClient
                                      ->SendAsync(JsonRequest(HttpMethod::Post, "/auth/register", body))
                                      .get();
            const auto parsed = J::Parse(response.BodyText());
            const auto& data = UnwrapData(parsed);

            ForumSession session;
            session.token = J::String(data, "token");
            session.expiresAt = J::Int(data, "expires_at");
            session.tokenType = J::String(data, "token_type", "Bearer");
            if (const auto* user = data.Find("user"); user != nullptr) {
                session.user = ParseUser(*user);
            }
            SetBearerToken(session.token);
            return session;
        });
}

std::future<ForumUser> ForumApiClient::MeAsync() {
    return std::async(std::launch::async, [this] {
        const auto response =
            m_httpClient->SendAsync(JsonRequest(HttpMethod::Get, "/auth/me")).get();
        const auto parsed = J::Parse(response.BodyText());
        return ParseUser(UnwrapData(parsed));
    });
}

std::future<void> ForumApiClient::LogoutAsync() {
    return std::async(std::launch::async, [this] {
        (void)m_httpClient->SendAsync(JsonRequest(HttpMethod::Post, "/auth/logout", "{}")).get();
        SetBearerToken({});
    });
}

std::future<ForumPostPage> ForumApiClient::ListPostsAsync(Aemeath::Services::ForumPostQuery query) {
    return std::async(std::launch::async, [this, query = std::move(query)] {
        std::vector<std::pair<std::string, std::string>> params {
            { "page", std::to_string(query.page) },
            { "per_page", std::to_string(query.perPage) },
            { "sort", query.sort },
        };
        if (query.tag.has_value()) {
            params.emplace_back("tag", *query.tag);
        }
        if (query.author.has_value()) {
            params.emplace_back("author", *query.author);
        }
        if (query.keyword.has_value()) {
            params.emplace_back("q", *query.keyword);
        }

        const auto response = m_httpClient
                                  ->SendAsync(JsonRequest(HttpMethod::Get, WithQuery("/posts", params)))
                                  .get();
        const auto parsed = J::Parse(response.BodyText());
        const auto& data = UnwrapData(parsed);

        ForumPostPage page;
        page.posts = ParseArray<ForumPost>(data, "posts", ParsePost);
        page.page = static_cast<int>(J::Int(data, "page", query.page));
        page.perPage = static_cast<int>(J::Int(data, "per_page", query.perPage));
        page.total = static_cast<int>(J::Int(data, "total"));
        page.totalPages = static_cast<int>(J::Int(data, "total_pages"));
        page.hasMore = J::Bool(data, "has_more", page.page < page.totalPages);
        page.sort = J::String(data, "sort", query.sort);
        return page;
    });
}

std::future<ForumPost> ForumApiClient::GetPostAsync(std::int64_t id) {
    return std::async(std::launch::async, [this, id] {
        const auto response =
            m_httpClient
                ->SendAsync(JsonRequest(HttpMethod::Get, "/posts/" + std::to_string(id)))
                .get();
        const auto parsed = J::Parse(response.BodyText());
        return ParsePost(UnwrapData(parsed));
    });
}

std::future<ForumPost> ForumApiClient::CreatePostAsync(
    Aemeath::Services::CreateForumPostRequest request) {
    return std::async(std::launch::async, [this, request = std::move(request)] {
        J::Value::Array tags;
        for (const auto& tag : request.tags) {
            tags.emplace_back(tag);
        }
        J::Value::Array images;
        for (const auto& imageId : request.imageIds) {
            images.emplace_back(imageId);
        }
        const auto body = J::Stringify(J::Value::Object {
            { "title", request.title },
            { "content", request.content },
            { "tags", tags },
            { "images", images },
        });

        const auto response =
            m_httpClient->SendAsync(JsonRequest(HttpMethod::Post, "/posts", body)).get();
        const auto parsed = J::Parse(response.BodyText());
        return ParsePost(UnwrapData(parsed));
    });
}

std::future<ForumReplyPage> ForumApiClient::ListRepliesAsync(
    std::int64_t postId,
    int page,
    int perPage) {
    return std::async(std::launch::async, [this, postId, page, perPage] {
        const auto path = WithQuery(
            "/posts/" + std::to_string(postId) + "/replies",
            { { "page", std::to_string(page) }, { "per_page", std::to_string(perPage) } });

        const auto response =
            m_httpClient->SendAsync(JsonRequest(HttpMethod::Get, path)).get();
        const auto parsed = J::Parse(response.BodyText());
        const auto& data = UnwrapData(parsed);

        ForumReplyPage result;
        result.replies = ParseArray<ForumReply>(data, "replies", ParseReply);
        result.page = static_cast<int>(J::Int(data, "page", page));
        result.perPage = static_cast<int>(J::Int(data, "per_page", perPage));
        result.total = static_cast<int>(J::Int(data, "total"));
        result.totalPages = static_cast<int>(J::Int(data, "total_pages"));
        result.hasMore = J::Bool(data, "has_more", result.page < result.totalPages);
        return result;
    });
}

std::future<ForumReply> ForumApiClient::CreateReplyAsync(
    std::int64_t postId,
    Aemeath::Services::CreateForumReplyRequest request) {
    return std::async(std::launch::async, [this, postId, request = std::move(request)] {
        J::Value::Object body {
            { "content", request.content },
        };
        if (request.parentId.has_value()) {
            body["parent_id"] = *request.parentId;
        }
        J::Value::Array images;
        for (const auto& imageId : request.imageIds) {
            images.emplace_back(imageId);
        }
        body["images"] = images;

        const auto response = m_httpClient
                                  ->SendAsync(JsonRequest(
                                      HttpMethod::Post,
                                      "/posts/" + std::to_string(postId) + "/replies",
                                      J::Stringify(body)))
                                  .get();
        const auto parsed = J::Parse(response.BodyText());
        return ParseReply(UnwrapData(parsed));
    });
}

std::future<ForumLikeResult> ForumApiClient::LikePostAsync(std::int64_t id, bool liked) {
    return std::async(std::launch::async, [this, id, liked] {
        const auto response = m_httpClient
                                  ->SendAsync(JsonRequest(
                                      liked ? HttpMethod::Post : HttpMethod::Delete,
                                      "/posts/" + std::to_string(id) + "/like",
                                      "{}"))
                                  .get();
        const auto parsed = J::Parse(response.BodyText());
        const auto& data = UnwrapData(parsed);
        return ForumLikeResult {
            J::Bool(data, "liked", liked),
            static_cast<int>(J::Int(data, "like_count")),
        };
    });
}

std::future<ForumLikeResult> ForumApiClient::LikeReplyAsync(std::int64_t id, bool liked) {
    return std::async(std::launch::async, [this, id, liked] {
        const auto response = m_httpClient
                                  ->SendAsync(JsonRequest(
                                      liked ? HttpMethod::Post : HttpMethod::Delete,
                                      "/replies/" + std::to_string(id) + "/like",
                                      "{}"))
                                  .get();
        const auto parsed = J::Parse(response.BodyText());
        const auto& data = UnwrapData(parsed);
        return ForumLikeResult {
            J::Bool(data, "liked", liked),
            static_cast<int>(J::Int(data, "like_count")),
        };
    });
}

std::future<std::vector<ForumTag>> ForumApiClient::ListTagsAsync(int limit) {
    return std::async(std::launch::async, [this, limit] {
        const auto response = m_httpClient
                                  ->SendAsync(JsonRequest(
                                      HttpMethod::Get,
                                      WithQuery("/tags", { { "limit", std::to_string(limit) } })))
                                  .get();
        const auto parsed = J::Parse(response.BodyText());
        const auto& data = UnwrapData(parsed);

        std::vector<ForumTag> tags;
        const J::Value* array = data.IsArray() ? &data : data.Find("tags");
        if (array != nullptr && array->IsArray()) {
            for (const auto& item : array->AsArray()) {
                tags.push_back(ForumTag {
                    J::String(item, "name"),
                    static_cast<int>(J::Int(item, "count")),
                });
            }
        }
        return tags;
    });
}

std::future<ForumImage> ForumApiClient::UploadImageAsync(std::string dataUrlOrBase64) {
    return std::async(std::launch::async, [this, dataUrlOrBase64 = std::move(dataUrlOrBase64)] {
        const auto body = J::Stringify(J::Value::Object {
            { "data", dataUrlOrBase64 },
        });
        const auto response =
            m_httpClient->SendAsync(JsonRequest(HttpMethod::Post, "/images", body)).get();
        const auto parsed = J::Parse(response.BodyText());
        return ParseImage(UnwrapData(parsed));
    });
}

} // namespace Aemeath::Infrastructure::Forum