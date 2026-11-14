#pragma once

#include <string>
#include <vector>

namespace Aemeath::Core::Config {

struct GeneralSettings {
    bool startMinimized { false };
    bool showMainWindowOnLaunch { false };
    std::string language { "zh-CN" };
    std::string logLevel { "info" };
    bool enableCrashDump { true };
};

struct PetSettings {
    bool topMost { true };
    bool clickThrough { false };
    double scale { 1.0 };
    int x { 100 };
    int y { 100 };
    std::string characterId { "default" };
    bool enableRandomActions { true };
    int idleActionIntervalSeconds { 30 };
    bool edgeSnap { false };
    double opacity { 1.0 };
};

struct ChatSettings {
    bool enabled { true };
    std::string provider { "mock" };
    std::string endpoint;
    std::string apiKeySecretName { "aemeath.chat.apiKey" };
    std::string model;
    std::string personaPath { "assets/persona/default.txt" };
    int maxHistoryMessages { 30 };
    double temperature { 0.7 };
    bool stream { true };
    int timeoutSeconds { 60 };
    bool enableToolCalls { false };
};

struct ForumSettings {
    bool enabled { true };
    std::string baseUrl { "https://forum.fxxr.store/api" };
    std::string tokenSecretName { "aemeath.forum.token" };
    int pageSize { 20 };
};

struct WallSettings {
    bool enabled { true };
    std::string mode { "forum" };
    std::string endpoint { "https://forum.fxxr.store/api" };
    std::string nickname { "访客" };
    bool autoRefresh { true };
    int refreshIntervalSeconds { 60 };
    int maxLocalMessages { 500 };
};

struct VoiceSettings {
    bool textToSpeechEnabled { false };
    bool speechRecognitionEnabled { false };
    std::string language { "zh-CN" };
    std::string ttsProvider { "windows" };
    std::string sttProvider { "windows" };
    std::string voiceId;
    double speechRate { 1.0 };
    double pitch { 1.0 };
    bool autoSpeakChatReply { false };
    bool pushToTalk { true };
    bool wakeWordEnabled { false };
    std::string localModelPath;
};

struct MusicSettings {
    bool enabled { false };
    double volume { 0.7 };
    std::vector<std::string> localFolders;
    std::string playMode { "loop" };
    std::string onlineProvider { "none" };
    bool cacheLyrics { true };
    bool showLyricBubble { false };
};

struct UpdateSettings {
    bool enabled { true };
    std::string channel { "stable" };
    bool checkOnStartup { true };
    int checkIntervalHours { 24 };
    std::string manifestUrl;
    bool autoDownload { false };
    bool autoInstall { false };
};

struct PluginSettings {
    bool enabled { false };
    std::vector<std::string> pluginDirectories { "assets/plugins" };
    bool allowNativePlugins { false };
    bool allowNetworkAccess { false };
    bool developerMode { false };
};

struct AppSettings {
    int schemaVersion { 2 };
    GeneralSettings app;
    PetSettings pet;
    ChatSettings chat;
    ForumSettings forum;
    WallSettings wall;
    VoiceSettings voice;
    MusicSettings music;
    UpdateSettings update;
    PluginSettings plugins;
};

} // namespace Aemeath::Core::Config
