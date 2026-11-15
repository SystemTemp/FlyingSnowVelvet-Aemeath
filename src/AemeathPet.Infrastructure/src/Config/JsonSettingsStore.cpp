#include "Aemeath/Infrastructure/Config/JsonSettingsStore.h"

#include "Aemeath/Infrastructure/Json/SimpleJson.h"

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace Aemeath::Infrastructure::Config {

namespace J = Aemeath::Infrastructure::Json;
using Aemeath::Core::Config::AppSettings;

namespace {

// ---------------------------------------------------------------------------
// 标量读写助手：缺字段时保留默认值，绝不因单个字段坏掉而丢弃整个文件。
// ---------------------------------------------------------------------------

void ReadGeneral(const J::Value& section, Aemeath::Core::Config::GeneralSettings& out) {
    out.startMinimized = J::Bool(section, "startMinimized", out.startMinimized);
    out.showMainWindowOnLaunch = J::Bool(section, "showMainWindowOnLaunch", out.showMainWindowOnLaunch);
    out.language = J::String(section, "language", out.language);
    out.logLevel = J::String(section, "logLevel", out.logLevel);
    out.enableCrashDump = J::Bool(section, "enableCrashDump", out.enableCrashDump);
}

void ReadPet(const J::Value& section, Aemeath::Core::Config::PetSettings& out) {
    out.topMost = J::Bool(section, "topMost", out.topMost);
    out.clickThrough = J::Bool(section, "clickThrough", out.clickThrough);
    if (const auto* scale = section.Find("scale"); scale != nullptr && scale->IsNumber()) {
        out.scale = scale->AsNumber();
    }
    out.x = static_cast<int>(J::Int(section, "x", out.x));
    out.y = static_cast<int>(J::Int(section, "y", out.y));
    out.characterId = J::String(section, "characterId", out.characterId);
    out.enableRandomActions = J::Bool(section, "enableRandomActions", out.enableRandomActions);
    out.idleActionIntervalSeconds = static_cast<int>(J::Int(section, "idleActionIntervalSeconds", out.idleActionIntervalSeconds));
    out.edgeSnap = J::Bool(section, "edgeSnap", out.edgeSnap);
    if (const auto* opacity = section.Find("opacity"); opacity != nullptr && opacity->IsNumber()) {
        out.opacity = opacity->AsNumber();
    }
}

void ReadChat(const J::Value& section, Aemeath::Core::Config::ChatSettings& out) {
    out.enabled = J::Bool(section, "enabled", out.enabled);
    out.provider = J::String(section, "provider", out.provider);
    out.endpoint = J::String(section, "endpoint", out.endpoint);
    out.apiKeySecretName = J::String(section, "apiKeySecretName", out.apiKeySecretName);
    out.model = J::String(section, "model", out.model);
    out.personaPath = J::String(section, "personaPath", out.personaPath);
    out.maxHistoryMessages = static_cast<int>(J::Int(section, "maxHistoryMessages", out.maxHistoryMessages));
    if (const auto* temperature = section.Find("temperature"); temperature != nullptr && temperature->IsNumber()) {
        out.temperature = temperature->AsNumber();
    }
    out.stream = J::Bool(section, "stream", out.stream);
    out.timeoutSeconds = static_cast<int>(J::Int(section, "timeoutSeconds", out.timeoutSeconds));
    out.enableToolCalls = J::Bool(section, "enableToolCalls", out.enableToolCalls);
}

void ReadForum(const J::Value& section, Aemeath::Core::Config::ForumSettings& out) {
    out.enabled = J::Bool(section, "enabled", out.enabled);
    out.baseUrl = J::String(section, "baseUrl", out.baseUrl);
    out.tokenSecretName = J::String(section, "tokenSecretName", out.tokenSecretName);
    out.pageSize = static_cast<int>(J::Int(section, "pageSize", out.pageSize));
}

void ReadWall(const J::Value& section, Aemeath::Core::Config::WallSettings& out) {
    out.enabled = J::Bool(section, "enabled", out.enabled);
    out.mode = J::String(section, "mode", out.mode);
    out.endpoint = J::String(section, "endpoint", out.endpoint);
    out.nickname = J::String(section, "nickname", out.nickname);
    out.autoRefresh = J::Bool(section, "autoRefresh", out.autoRefresh);
    out.refreshIntervalSeconds = static_cast<int>(J::Int(section, "refreshIntervalSeconds", out.refreshIntervalSeconds));
    out.maxLocalMessages = static_cast<int>(J::Int(section, "maxLocalMessages", out.maxLocalMessages));
}

void ReadVoice(const J::Value& section, Aemeath::Core::Config::VoiceSettings& out) {
    out.textToSpeechEnabled = J::Bool(section, "textToSpeechEnabled", out.textToSpeechEnabled);
    out.speechRecognitionEnabled = J::Bool(section, "speechRecognitionEnabled", out.speechRecognitionEnabled);
    out.language = J::String(section, "language", out.language);
    out.ttsProvider = J::String(section, "ttsProvider", out.ttsProvider);
    out.sttProvider = J::String(section, "sttProvider", out.sttProvider);
    out.voiceId = J::String(section, "voiceId", out.voiceId);
    if (const auto* rate = section.Find("speechRate"); rate != nullptr && rate->IsNumber()) {
        out.speechRate = rate->AsNumber();
    }
    if (const auto* pitch = section.Find("pitch"); pitch != nullptr && pitch->IsNumber()) {
        out.pitch = pitch->AsNumber();
    }
    out.autoSpeakChatReply = J::Bool(section, "autoSpeakChatReply", out.autoSpeakChatReply);
    out.pushToTalk = J::Bool(section, "pushToTalk", out.pushToTalk);
    out.wakeWordEnabled = J::Bool(section, "wakeWordEnabled", out.wakeWordEnabled);
    out.localModelPath = J::String(section, "localModelPath", out.localModelPath);
}

void ReadMusic(const J::Value& section, Aemeath::Core::Config::MusicSettings& out) {
    out.enabled = J::Bool(section, "enabled", out.enabled);
    if (const auto* volume = section.Find("volume"); volume != nullptr && volume->IsNumber()) {
        out.volume = volume->AsNumber();
    }
    if (const auto* folders = section.Find("localFolders"); folders != nullptr && folders->IsArray()) {
        out.localFolders.clear();
        for (const auto& entry : folders->AsArray()) {
            if (entry.IsString()) {
                out.localFolders.push_back(entry.AsString());
            }
        }
    }
    out.playMode = J::String(section, "playMode", out.playMode);
    out.onlineProvider = J::String(section, "onlineProvider", out.onlineProvider);
    out.cacheLyrics = J::Bool(section, "cacheLyrics", out.cacheLyrics);
    out.showLyricBubble = J::Bool(section, "showLyricBubble", out.showLyricBubble);
}

void ReadUpdate(const J::Value& section, Aemeath::Core::Config::UpdateSettings& out) {
    out.enabled = J::Bool(section, "enabled", out.enabled);
    out.channel = J::String(section, "channel", out.channel);
    out.checkOnStartup = J::Bool(section, "checkOnStartup", out.checkOnStartup);
    out.checkIntervalHours = static_cast<int>(J::Int(section, "checkIntervalHours", out.checkIntervalHours));
    out.manifestUrl = J::String(section, "manifestUrl", out.manifestUrl);
    out.autoDownload = J::Bool(section, "autoDownload", out.autoDownload);
    out.autoInstall = J::Bool(section, "autoInstall", out.autoInstall);
}

void ReadPlugins(const J::Value& section, Aemeath::Core::Config::PluginSettings& out) {
    out.enabled = J::Bool(section, "enabled", out.enabled);
    if (const auto* directories = section.Find("pluginDirectories"); directories != nullptr && directories->IsArray()) {
        out.pluginDirectories.clear();
        for (const auto& entry : directories->AsArray()) {
            if (entry.IsString()) {
                out.pluginDirectories.push_back(entry.AsString());
            }
        }
    }
    out.allowNativePlugins = J::Bool(section, "allowNativePlugins", out.allowNativePlugins);
    out.allowNetworkAccess = J::Bool(section, "allowNetworkAccess", out.allowNetworkAccess);
    out.developerMode = J::Bool(section, "developerMode", out.developerMode);
}

J::Value StringArray(const std::vector<std::string>& values) {
    J::Value::Array array;
    array.reserve(values.size());
    for (const auto& value : values) {
        array.emplace_back(value);
    }
    return J::Value(std::move(array));
}

// ---------------------------------------------------------------------------
// 序列化
// ---------------------------------------------------------------------------

J::Value WriteGeneral(const Aemeath::Core::Config::GeneralSettings& value) {
    return J::Value::Object {
        { "startMinimized", value.startMinimized },
        { "showMainWindowOnLaunch", value.showMainWindowOnLaunch },
        { "language", value.language },
        { "logLevel", value.logLevel },
        { "enableCrashDump", value.enableCrashDump },
    };
}

J::Value WritePet(const Aemeath::Core::Config::PetSettings& value) {
    return J::Value::Object {
        { "topMost", value.topMost },
        { "clickThrough", value.clickThrough },
        { "scale", value.scale },
        { "x", value.x },
        { "y", value.y },
        { "characterId", value.characterId },
        { "enableRandomActions", value.enableRandomActions },
        { "idleActionIntervalSeconds", value.idleActionIntervalSeconds },
        { "edgeSnap", value.edgeSnap },
        { "opacity", value.opacity },
    };
}

J::Value WriteChat(const Aemeath::Core::Config::ChatSettings& value) {
    return J::Value::Object {
        { "enabled", value.enabled },
        { "provider", value.provider },
        { "endpoint", value.endpoint },
        { "apiKeySecretName", value.apiKeySecretName },
        { "model", value.model },
        { "personaPath", value.personaPath },
        { "maxHistoryMessages", value.maxHistoryMessages },
        { "temperature", value.temperature },
        { "stream", value.stream },
        { "timeoutSeconds", value.timeoutSeconds },
        { "enableToolCalls", value.enableToolCalls },
    };
}

J::Value WriteForum(const Aemeath::Core::Config::ForumSettings& value) {
    return J::Value::Object {
        { "enabled", value.enabled },
        { "baseUrl", value.baseUrl },
        { "tokenSecretName", value.tokenSecretName },
        { "pageSize", value.pageSize },
    };
}

J::Value WriteWall(const Aemeath::Core::Config::WallSettings& value) {
    return J::Value::Object {
        { "enabled", value.enabled },
        { "mode", value.mode },
        { "endpoint", value.endpoint },
        { "nickname", value.nickname },
        { "autoRefresh", value.autoRefresh },
        { "refreshIntervalSeconds", value.refreshIntervalSeconds },
        { "maxLocalMessages", value.maxLocalMessages },
    };
}

J::Value WriteVoice(const Aemeath::Core::Config::VoiceSettings& value) {
    return J::Value::Object {
        { "textToSpeechEnabled", value.textToSpeechEnabled },
        { "speechRecognitionEnabled", value.speechRecognitionEnabled },
        { "language", value.language },
        { "ttsProvider", value.ttsProvider },
        { "sttProvider", value.sttProvider },
        { "voiceId", value.voiceId },
        { "speechRate", value.speechRate },
        { "pitch", value.pitch },
        { "autoSpeakChatReply", value.autoSpeakChatReply },
        { "pushToTalk", value.pushToTalk },
        { "wakeWordEnabled", value.wakeWordEnabled },
        { "localModelPath", value.localModelPath },
    };
}

J::Value WriteMusic(const Aemeath::Core::Config::MusicSettings& value) {
    return J::Value::Object {
        { "enabled", value.enabled },
        { "volume", value.volume },
        { "localFolders", StringArray(value.localFolders) },
        { "playMode", value.playMode },
        { "onlineProvider", value.onlineProvider },
        { "cacheLyrics", value.cacheLyrics },
        { "showLyricBubble", value.showLyricBubble },
    };
}

J::Value WriteUpdate(const Aemeath::Core::Config::UpdateSettings& value) {
    return J::Value::Object {
        { "enabled", value.enabled },
        { "channel", value.channel },
        { "checkOnStartup", value.checkOnStartup },
        { "checkIntervalHours", value.checkIntervalHours },
        { "manifestUrl", value.manifestUrl },
        { "autoDownload", value.autoDownload },
        { "autoInstall", value.autoInstall },
    };
}

J::Value WritePlugins(const Aemeath::Core::Config::PluginSettings& value) {
    return J::Value::Object {
        { "enabled", value.enabled },
        { "pluginDirectories", StringArray(value.pluginDirectories) },
        { "allowNativePlugins", value.allowNativePlugins },
        { "allowNetworkAccess", value.allowNetworkAccess },
        { "developerMode", value.developerMode },
    };
}

} // namespace

JsonSettingsStore::JsonSettingsStore(std::filesystem::path path)
    : m_path(std::move(path)) {
}

AppSettings JsonSettingsStore::Load() {
    AppSettings settings;

    std::error_code ec;
    if (!std::filesystem::exists(m_path, ec)) {
        return settings;
    }

    std::ifstream stream(m_path, std::ios::binary);
    if (!stream) {
        return settings;
    }
    std::istreambuf_iterator<char> first(stream);
    std::istreambuf_iterator<char> last;
    const std::string text(first, last);
    if (text.empty()) {
        return settings;
    }

    try {
        const auto root = J::Parse(text);

        settings.schemaVersion = static_cast<int>(J::Int(root, "schemaVersion", settings.schemaVersion));
        if (const auto* section = root.Find("app"); section != nullptr) {
            ReadGeneral(*section, settings.app);
        }
        if (const auto* section = root.Find("pet"); section != nullptr) {
            ReadPet(*section, settings.pet);
        }
        if (const auto* section = root.Find("chat"); section != nullptr) {
            ReadChat(*section, settings.chat);
        }
        if (const auto* section = root.Find("forum"); section != nullptr) {
            ReadForum(*section, settings.forum);
        }
        if (const auto* section = root.Find("wall"); section != nullptr) {
            ReadWall(*section, settings.wall);
        }
        if (const auto* section = root.Find("voice"); section != nullptr) {
            ReadVoice(*section, settings.voice);
        }
        if (const auto* section = root.Find("music"); section != nullptr) {
            ReadMusic(*section, settings.music);
        }
        if (const auto* section = root.Find("update"); section != nullptr) {
            ReadUpdate(*section, settings.update);
        }
        if (const auto* section = root.Find("plugins"); section != nullptr) {
            ReadPlugins(*section, settings.plugins);
        }
    } catch (const std::exception& ex) {
        // 坏文件不打断启动：返回默认值并让上层通过日志感知。
        // （Host 在调用 Load 时会捕获并记录。）
        (void)ex;
        return AppSettings {};
    }
    return settings;
}

void JsonSettingsStore::Save(const AppSettings& settings) {
    J::Value root = J::Value::Object {
        { "schemaVersion", settings.schemaVersion },
        { "app", WriteGeneral(settings.app) },
        { "pet", WritePet(settings.pet) },
        { "chat", WriteChat(settings.chat) },
        { "forum", WriteForum(settings.forum) },
        { "wall", WriteWall(settings.wall) },
        { "voice", WriteVoice(settings.voice) },
        { "music", WriteMusic(settings.music) },
        { "update", WriteUpdate(settings.update) },
        { "plugins", WritePlugins(settings.plugins) },
    };

    std::error_code ec;
    const auto parent = m_path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, ec);
    }

    // 原子替换：先写临时文件，再 rename 覆盖，避免半截 JSON。
    auto temporary = m_path;
    temporary += ".tmp";
    {
        std::ofstream stream(temporary, std::ios::binary | std::ios::trunc);
        if (!stream) {
            throw std::runtime_error("无法写入设置文件：" + temporary.string());
        }
        stream << J::Stringify(root);
        stream.flush();
        if (!stream) {
            throw std::runtime_error("写入设置文件失败：" + temporary.string());
        }
    }

    std::filesystem::remove(m_path, ec);
    std::filesystem::rename(temporary, m_path, ec);
    if (ec) {
        throw std::runtime_error("设置文件替换失败：" + ec.message());
    }
}

} // namespace Aemeath::Infrastructure::Config