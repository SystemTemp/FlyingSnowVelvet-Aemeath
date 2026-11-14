#pragma once

#include "Aemeath/Core/Music/MusicModels.h"
#include <string_view>
#include <vector>

namespace Aemeath::Services {

class IMusicPlayerService {
public:
    virtual ~IMusicPlayerService() = default;
    virtual void Play(const Aemeath::Core::Music::Track& track) = 0;
    virtual void Pause() = 0;
    virtual void Resume() = 0;
    virtual void Stop() = 0;
    virtual void SetVolume(double volume) = 0;
    virtual Aemeath::Core::Music::PlaybackStatus Status() const = 0;
};

class IMusicProvider {
public:
    virtual ~IMusicProvider() = default;
    virtual std::string Name() const = 0;
    virtual std::vector<Aemeath::Core::Music::Track> Search(std::string_view keyword, int page, int pageSize) = 0;
};

} // namespace Aemeath::Services
