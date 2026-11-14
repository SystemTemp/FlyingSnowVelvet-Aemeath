#pragma once

#include <string>
#include <vector>

namespace Aemeath::Core::Plugins {

enum class PluginState {
    Disabled,
    Enabled,
    Error
};

struct PluginManifest {
    std::string id;
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    std::vector<std::string> permissions;
};

} // namespace Aemeath::Core::Plugins
