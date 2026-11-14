#pragma once

#include <string>

namespace Aemeath::Core::Update {

struct AppVersion {
    int major { 0 };
    int minor { 0 };
    int patch { 0 };
    std::string suffix;
};

struct UpdateManifest {
    std::string version;
    std::string channel;
    std::string releaseNotes;
    std::string downloadUrl;
    std::string sha256;
    bool mandatory { false };
};

} // namespace Aemeath::Core::Update
