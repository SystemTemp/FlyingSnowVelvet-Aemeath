#pragma once

#include "Aemeath/Core/Forum/ForumModels.h"
#include <future>
#include <optional>
#include <string>
#include <vector>

namespace Aemeath::Services {

struct ForumPostQuery {
    int page { 1 };
    int perPage { 20 };
    std::string sort { "new" };
    std::optional<std::string> tag;
    std::optional<std::string> author;
    std::optional<std::string> keyword;
};

struct CreateForumPostRequest {
    std::string title;
    std::string content;
    std::vector<std::string> tags;
    std::vector<std::string> imageIds;
};

struct CreateForumReplyRequest {
    std::string content;
    std::optional<std::int64_t> parentId;
    std::vector<std::string> imageIds;
};

class IForumService {
public:
    virtual ~IForumService() = default;

    virtual std::future<Aemeath::Core::Forum::ForumSession> LoginAsync(
        std::string username,
        std::string password) = 0;

    virtual std::future<Aemeath::Core::Forum::ForumSession> RegisterAsync(
        std::string username,
        std::string password,
        std::string displayName) = 0;

    virtual std::future<Aemeath::Core::Forum::ForumUser> MeAsync() = 0;
    virtual std::future<void> LogoutAsync() = 0;

    virtual std::future<Aemeath::Core::Forum::ForumPostPage> ListPostsAsync(ForumPostQuery query) = 0;
    virtual std::future<Aemeath::Core::Forum::ForumPost> GetPostAsync(std::int64_t id) = 0;
    virtual std::future<Aemeath::Core::Forum::ForumPost> CreatePostAsync(CreateForumPostRequest request) = 0;

    virtual std::future<Aemeath::Core::Forum::ForumReplyPage> ListRepliesAsync(
        std::int64_t postId,
        int page,
        int perPage) = 0;

    virtual std::future<Aemeath::Core::Forum::ForumReply> CreateReplyAsync(
        std::int64_t postId,
        CreateForumReplyRequest request) = 0;

    virtual std::future<Aemeath::Core::Forum::ForumLikeResult> LikePostAsync(std::int64_t id, bool liked) = 0;
    virtual std::future<Aemeath::Core::Forum::ForumLikeResult> LikeReplyAsync(std::int64_t id, bool liked) = 0;

    virtual std::future<std::vector<Aemeath::Core::Forum::ForumTag>> ListTagsAsync(int limit) = 0;
    virtual std::future<Aemeath::Core::Forum::ForumImage> UploadImageAsync(std::string dataUrlOrBase64) = 0;
};

} // namespace Aemeath::Services
