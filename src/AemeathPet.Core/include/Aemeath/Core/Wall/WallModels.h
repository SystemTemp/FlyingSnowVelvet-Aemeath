#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace Aemeath::Core::Wall {

enum class WallMessageStatus {
    LocalOnly,
    PendingSync,
    Synced,
    SyncedError
};

struct WallReaction {
    std::string type;
    int count { 0 };
    bool reactedByMe { false };
};

struct WallMessage {
    std::string id;
    std::string remoteId;
    std::string author;
    std::string content;
    std::vector<std::string> imageIds;
    std::vector<std::string> tags;
    WallMessageStatus status { WallMessageStatus::LocalOnly };
    std::vector<WallReaction> reactions;
    std::chrono::system_clock::time_point createdAt { std::chrono::system_clock::now() };
    std::chrono::system_clock::time_point updatedAt { std::chrono::system_clock::now() };
};

struct WallMessagePage {
    std::vector<WallMessage> messages;
    std::string nextCursor;
    bool hasMore { false };
};

} // namespace Aemeath::Core::Wall
