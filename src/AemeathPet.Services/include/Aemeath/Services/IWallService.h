#pragma once

#include "Aemeath/Core/Wall/WallModels.h"
#include <future>
#include <optional>
#include <string>
#include <vector>

namespace Aemeath::Services {

class IWallMessageStore {
public:
    virtual ~IWallMessageStore() = default;
    virtual std::vector<Aemeath::Core::Wall::WallMessage> ListLocal(int limit, int offset) = 0;
    virtual void SaveLocal(const Aemeath::Core::Wall::WallMessage& message) = 0;
};

class IWallService {
public:
    virtual ~IWallService() = default;
    virtual std::future<Aemeath::Core::Wall::WallMessagePage> FetchAsync(int limit, std::string cursor) = 0;
    virtual std::future<Aemeath::Core::Wall::WallMessage> PostAsync(Aemeath::Core::Wall::WallMessage message) = 0;
};

} // namespace Aemeath::Services
