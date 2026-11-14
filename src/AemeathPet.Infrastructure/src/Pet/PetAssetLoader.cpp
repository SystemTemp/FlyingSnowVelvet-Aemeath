#include "Aemeath/Infrastructure/Pet/PetAssetLoader.h"

#include "Aemeath/Infrastructure/Json/SimpleJson.h"

#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace Aemeath::Infrastructure::Pet {

namespace J = Aemeath::Infrastructure::Json;
using Aemeath::Core::Pet::AnimationClip;
using Aemeath::Core::Pet::AnimationLibrary;

namespace {

std::string ReadTextFile(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return {};
    }
    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

void ParseClip(const J::Value& source, const std::string& id, AnimationLibrary& library) {
    AnimationClip clip;
    clip.id = id;
    clip.fps = static_cast<int>(J::Int(source, "fps", 8));
    clip.loop = J::Bool(source, "loop", true);

    if (const auto* frames = source.Find("frames"); frames != nullptr && frames->IsArray()) {
        for (const auto& entry : frames->AsArray()) {
            if (entry.IsString()) {
                clip.frames.push_back(entry.AsString());
            }
        }
    } else if (const auto* single = source.Find("frame"); single != nullptr && single->IsString()) {
        clip.frames.push_back(single->AsString());
    }

    if (clip.fps <= 0) {
        clip.fps = 8;
    }
    library.clips[id] = std::move(clip);
}

} // namespace

AnimationLibrary PetAssetLoader::Load(const std::filesystem::path& manifestPath, std::string* error) {
    AnimationLibrary library;

    const auto text = ReadTextFile(manifestPath);
    if (text.empty()) {
        if (error != nullptr) {
            *error = "无法读取资源清单：" + manifestPath.string();
        }
        return library;
    }

    try {
        const auto root = J::Parse(text);
        library.petId = J::String(root, "id", manifestPath.parent_path().filename().string());
        library.displayName = J::String(root, "name", library.petId);
        if (const auto* scale = root.Find("scale"); scale != nullptr && scale->IsNumber()) {
            library.scale = scale->AsNumber();
        }
        if (library.scale <= 0.05 || library.scale > 8.0) {
            library.scale = 1.0;
        }
        library.baseDirectory = manifestPath.parent_path();

        if (const auto* animations = root.Find("animations"); animations != nullptr && animations->IsObject()) {
            for (const auto& [id, value] : animations->AsObject()) {
                ParseClip(value, id, library);
            }
        }
    } catch (const std::exception& ex) {
        if (error != nullptr) {
            *error = std::string("资源清单解析失败：") + ex.what();
        }
        return {};
    }

    if (library.clips.empty() && error != nullptr) {
        *error = "资源清单里没有任何动画片段：" + manifestPath.string();
    }
    return library;
}

std::optional<AnimationLibrary> PetAssetLoader::LoadPreferred(
    const std::filesystem::path& assetsRoot,
    std::string_view preferredId,
    std::string* error) {
    const auto petRoot = assetsRoot / "pet";
    std::error_code ec;

    if (!preferredId.empty()) {
        const auto candidate = petRoot / std::string(preferredId) / "manifest.json";
        if (std::filesystem::exists(candidate, ec)) {
            auto library = Load(candidate, error);
            if (!library.clips.empty()) {
                return library;
            }
        }
    }

    const auto defaultManifest = petRoot / "default" / "manifest.json";
    if (std::filesystem::exists(defaultManifest, ec)) {
        auto library = Load(defaultManifest, error);
        if (!library.clips.empty()) {
            return library;
        }
    }

    if (std::filesystem::is_directory(petRoot, ec)) {
        for (const auto& entry : std::filesystem::directory_iterator(petRoot, ec)) {
            if (!entry.is_directory()) {
                continue;
            }
            const auto manifest = entry.path() / "manifest.json";
            if (!std::filesystem::exists(manifest, ec)) {
                continue;
            }
            auto library = Load(manifest, error);
            if (!library.clips.empty()) {
                return library;
            }
        }
    }

    if (error != nullptr && error->empty()) {
        *error = "没有找到任何宠物资源清单（期望 " + (petRoot / "default" / "manifest.json").string() + "）";
    }
    return std::nullopt;
}

std::filesystem::path PetAssetLoader::ResolveFrame(
    const AnimationLibrary& library,
    std::string_view frame) {
    if (frame.empty()) {
        return {};
    }
    std::filesystem::path relativePath { std::string(frame) };
    if (relativePath.is_absolute()) {
        return relativePath;
    }
    return library.baseDirectory / relativePath;
}

} // namespace Aemeath::Infrastructure::Pet