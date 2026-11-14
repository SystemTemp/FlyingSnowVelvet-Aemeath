#pragma once

#include "Aemeath/Services/IForumService.h"
#include "Aemeath/Services/IHttpClient.h"
#include <memory>
#include <mutex>

namespace Aemeath::Infrastructure::Forum {

class ForumApiClient final : public Aemeath::Services::IForumService {
public:
    explicit ForumApiClient(
        std::shared_ptr<Aemeath::Services::IHttpClient> httpClient,
        std::string baseUrl = "https://forum.fxxr.store/api");

    void SetBearerToken(std::string token);
    [[nodiscard]] const std::string& BaseUrl() const noexcept { return m_baseUrl; }

    std::future<Aemeath::Core::Forum::ForumSession> LoginAsync(std::string username, std::string password) override;
    std::future<Aemeath::Core::Forum::ForumSession> RegisterAsync(std::string username, std::string password, std::string displayName) override;
    std::future<Aemeath::Core::Forum::ForumUser> MeAsync() override;
    std::future<void> LogoutAsync() override;

    std::future<Aemeath::Core::Forum::ForumPostPage> ListPostsAsync(Aemeath::Services::ForumPostQuery query) override;
    std::future<Aemeath::Core::Forum::ForumPost> GetPostAsync(std::int64_t id) override;
    std::future<Aemeath::Core::Forum::ForumPost> CreatePostAsync(Aemeath::Services::CreateForumPostRequest request) override;

    std::future<Aemeath::Core::Forum::ForumReplyPage> ListRepliesAsync(std::int64_t postId, int page, int perPage) override;
    std::future<Aemeath::Core::Forum::ForumReply> CreateReplyAsync(std::int64_t postId, Aemeath::Services::CreateForumReplyRequest request) override;

    std::future<Aemeath::Core::Forum::ForumLikeResult> LikePostAsync(std::int64_t id, bool liked) override;
    std::future<Aemeath::Core::Forum::ForumLikeResult> LikeReplyAsync(std::int64_t id, bool liked) override;

    std::future<std::vector<Aemeath::Core::Forum::ForumTag>> ListTagsAsync(int limit) override;
    std::future<Aemeath::Core::Forum::ForumImage> UploadImageAsync(std::string dataUrlOrBase64) override;

private:
    Aemeath::Core::Network::HttpRequest JsonRequest(Aemeath::Core::Network::HttpMethod method, std::string path, std::string body = {}) const;
    std::string WithQuery(std::string path, const std::vector<std::pair<std::string, std::string>>& query) const;

    std::shared_ptr<Aemeath::Services::IHttpClient> m_httpClient;
    std::string m_baseUrl;
    mutable std::mutex m_tokenMutex;
    std::string m_bearerToken;
};

} // namespace Aemeath::Infrastructure::Forum
