#pragma once

#include "Aemeath/Core/Config/AppSettings.h"

namespace Aemeath::Services {

class ISettingsStore {
public:
    virtual ~ISettingsStore() = default;
    virtual Aemeath::Core::Config::AppSettings Load() = 0;
    virtual void Save(const Aemeath::Core::Config::AppSettings& settings) = 0;
};

} // namespace Aemeath::Services
