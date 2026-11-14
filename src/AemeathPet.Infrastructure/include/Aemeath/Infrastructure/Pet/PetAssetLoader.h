#pragma once

#include "Aemeath/Core/Pet/AnimationPlayer.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace Aemeath::Infrastructure::Pet {

/// 解析 assets/pet/<id>/manifest.json，产出 Core 的 AnimationLibrary。
class PetAssetLoader final {
public:
    static Aemeath::Core::Pet::AnimationLibrary Load(
        const std::filesystem::path& manifestPath,
        std::string* error = nullptr);

    /// 在资源根下优先找指定宠物，找不到则回退到 default，再回退到第一个含 manifest 的目录。
    static std::optional<Aemeath::Core::Pet::AnimationLibrary> LoadPreferred(
        const std::filesystem::path& assetsRoot,
        std::string_view preferredId,
        std::string* error = nullptr);

    /// 把清单里的相对帧路径解析成绝对路径。
    static std::filesystem::path ResolveFrame(
        const Aemeath::Core::Pet::AnimationLibrary& library,
        std::string_view frame);
};

} // namespace Aemeath::Infrastructure::Pet