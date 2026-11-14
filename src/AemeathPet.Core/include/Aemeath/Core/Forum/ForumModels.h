#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace Aemeath::Core::Forum {

struct ForumUser {
    std::int64_t id { 0 };
    std::string username;
    std::string displayName;
    std::string bio;
    int postCount { 0 };
    int replyCount { 0 };
    int likesGiven { 0 };
    std::int64_t createdAt { 0 };
};

struct ForumSession {
    std::string token;
    ForumUser user;
    std::int64_t expiresAt { 0 };
    std::string tokenType { "Bearer" };
};

struct ForumImage {
    std::string id;
    std::string url;
    std::string mime;
    int width { 0 };
    int height { 0 };
    std::int64_t size { 0 };
    std::int64_t createdAt { 0 };
};

struct ForumPost {
    std::int64_t id { 0 };
    std::string title;
    std::string content;
    std::string excerpt;
    std::vector<std::string> tags;
    ForumUser author;
    int replyCount { 0 };
    int likeCount { 0 };
    int viewCount { 0 };
    int imageCount { 0 };
    bool likedByMe { false };
    std::int64_t createdAt { 0 };
    std::int64_t updatedAt { 0 };
    std::int64_t lastReplyAt { 0 };
    std::vector<ForumImage> images;
};

struct ForumPostPage {
    std::vector<ForumPost> posts;
    int page { 1 };
    int perPage { 20 };
    int total { 0 };
    int totalPages { 0 };
    bool hasMore { false };
    std::string sort { "new" };
};

struct ForumReply {
    std::int64_t id { 0 };
    std::int64_t postId { 0 };
    std::optional<std::int64_t> parentId;
    std::string content;
    ForumUser author;
    int likeCount { 0 };
    bool likedByMe { false };
    int imageCount { 0 };
    std::int64_t createdAt { 0 };
    std::int64_t updatedAt { 0 };
    std::string postTitle;
    std::vector<ForumImage> images;
};

struct ForumReplyPage {
    std::vector<ForumReply> replies;
    int page { 1 };
    int perPage { 20 };
    int total { 0 };
    int totalPages { 0 };
    bool hasMore { false };
};

struct ForumTag {
    std::string name;
    int count { 0 };
};

struct ForumLikeResult {
    bool liked { false };
    int likeCount { 0 };
};

struct ForumError {
    std::string code;
    std::string message;
    std::string details;
};

} // namespace Aemeath::Core::Forum
