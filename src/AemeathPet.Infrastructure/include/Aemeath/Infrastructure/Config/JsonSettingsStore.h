#pragma once
#include "Aemeath/Services/ISettingsStore.h"
#include <filesystem>
namespace Aemeath::Infrastructure::Config {class JsonSettingsStore final:public Aemeath::Services::ISettingsStore{public:explicit JsonSettingsStore(std::filesystem::path path);Aemeath::Core::Config::AppSettings Load()override;void Save(const Aemeath::Core::Config::AppSettings&)override;private:std::filesystem::path m_path;};}
