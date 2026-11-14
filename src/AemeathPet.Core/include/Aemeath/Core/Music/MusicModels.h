#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace Aemeath::Core::Music {

enum class PlaybackStatus {
    Stopped,
    Playing,
    Paused,
    Buffering,
    Error
};

struct Track {
    std::string id;
    std::string title;
    std::string artist;
    std::string album;
    std::string uri;
    std::chrono::milliseconds duration { 0 };
};

struct Playlist {
    std::string id;
    std::string name;
    std::vector<Track> tracks;
    std::size_t currentIndex { 0 };
};

} // namespace Aemeath::Core::Music
