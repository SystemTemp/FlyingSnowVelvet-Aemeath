#include "TestSupport.h"

#include "Aemeath/Core/Config/AppSettings.h"
#include "Aemeath/Core/Platform/UserPaths.h"
#include "Aemeath/Infrastructure/Config/JsonSettingsStore.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

std::filesystem::path MakeTempDirectory() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    auto path = std::filesystem::temp_directory_path()
        / ("aemeath-test-" + std::to_string(stamp));
    std::filesystem::create_directories(path);
    return path;
}

AE_TEST(Settings_DefaultsAreSane) {
    Aemeath::Core::Config::AppSettings settings;
    AE_EXPECT_EQ(settings.pet.topMost, true);
    AE_EXPECT_EQ(settings.pet.clickThrough, false);
    AE_EXPECT(settings.pet.scale > 0.0);
    AE_EXPECT_EQ(settings.forum.baseUrl, std::string("https://forum.fxxr.store/api"));
    AE_EXPECT_EQ(settings.chat.personaPath, std::string("assets/persona/default.txt"));
}

AE_TEST(Settings_StoreRoundTrip) {
    const auto directory = MakeTempDirectory();
    const auto file = directory / "settings.json";

    Aemeath::Core::Config::AppSettings original;
    original.schemaVersion = 3;
    original.app.startMinimized = true;
    original.app.showMainWindowOnLaunch = true;
    original.app.language = "zh-CN";
    original.app.logLevel = "debug";
    original.app.enableCrashDump = false;

    original.pet.x = 1234;
    original.pet.y = 567;
    original.pet.scale = 1.5;
    original.pet.topMost = false;
    original.pet.clickThrough = true;
    original.pet.characterId = "snow";
    original.pet.enableRandomActions = false;
    original.pet.idleActionIntervalSeconds = 45;
    original.pet.edgeSnap = true;
    original.pet.opacity = 0.8;

    original.chat.provider = "openai";
    original.chat.endpoint = "https://api.example.com/v1";
    original.chat.model = "test-model";
    original.chat.personaPath = "assets/persona/custom.txt";
    original.chat.apiKeySecretName = "custom.key";
    original.chat.maxHistoryMessages = 12;
    original.chat.temperature = 0.42;
    original.chat.stream = false;
    original.chat.timeoutSeconds = 90;
    original.chat.enableToolCalls = true;
    original.chat.enabled = false;

    original.forum.baseUrl = "https://forum.example.com/api";
    original.forum.pageSize = 40;
    original.forum.tokenSecretName = "custom.forum";
    original.forum.enabled = false;

    original.wall.mode = "hybrid";
    original.wall.nickname = "Snow";
    original.wall.autoRefresh = false;
    original.wall.refreshIntervalSeconds = 120;
    original.wall.maxLocalMessages = 99;

    original.voice.textToSpeechEnabled = true;
    original.voice.speechRecognitionEnabled = true;
    original.voice.language = "ja-JP";
    original.voice.ttsProvider = "onnx";
    original.voice.sttProvider = "vosk";
    original.voice.voiceId = "voice-7";
    original.voice.speechRate = 1.25;
    original.voice.pitch = 0.9;
    original.voice.autoSpeakChatReply = true;
    original.voice.pushToTalk = false;
    original.voice.wakeWordEnabled = true;
    original.voice.localModelPath = "D:/models/vosk";

    original.music.enabled = true;
    original.music.volume = 0.35;
    original.music.localFolders = { "D:/music/a", "D:/music/b" };
    original.music.playMode = "shuffle";
    original.music.onlineProvider = "netease";
    original.music.cacheLyrics = false;
    original.music.showLyricBubble = true;

    original.update.enabled = false;
    original.update.channel = "beta";
    original.update.checkOnStartup = false;
    original.update.checkIntervalHours = 6;
    original.update.manifestUrl = "https://example.com/update.json";
    original.update.autoDownload = true;
    original.update.autoInstall = true;

    original.plugins.enabled = true;
    original.plugins.pluginDirectories = { "assets/plugins", "D:/plugins" };
    original.plugins.allowNativePlugins = true;
    original.plugins.allowNetworkAccess = true;
    original.plugins.developerMode = true;

    {
        Aemeath::Infrastructure::Config::JsonSettingsStore store(file);
        store.Save(original);
    }

    AE_EXPECT(std::filesystem::exists(file));

    Aemeath::Infrastructure::Config::JsonSettingsStore loaded(file);
    const auto restored = loaded.Load();

    AE_EXPECT_EQ(restored.schemaVersion, 3);
    AE_EXPECT_EQ(restored.app.startMinimized, true);
    AE_EXPECT_EQ(restored.app.showMainWindowOnLaunch, true);
    AE_EXPECT_EQ(restored.app.language, std::string("zh-CN"));
    AE_EXPECT_EQ(restored.app.logLevel, std::string("debug"));
    AE_EXPECT_EQ(restored.app.enableCrashDump, false);

    AE_EXPECT_EQ(restored.pet.x, 1234);
    AE_EXPECT_EQ(restored.pet.y, 567);
    AE_EXPECT(restored.pet.scale > 1.49 && restored.pet.scale < 1.51);
    AE_EXPECT_EQ(restored.pet.topMost, false);
    AE_EXPECT_EQ(restored.pet.clickThrough, true);
    AE_EXPECT_EQ(restored.pet.characterId, std::string("snow"));
    AE_EXPECT_EQ(restored.pet.enableRandomActions, false);
    AE_EXPECT_EQ(restored.pet.idleActionIntervalSeconds, 45);
    AE_EXPECT_EQ(restored.pet.edgeSnap, true);
    AE_EXPECT(restored.pet.opacity > 0.79 && restored.pet.opacity < 0.81);

    AE_EXPECT_EQ(restored.chat.provider, std::string("openai"));
    AE_EXPECT_EQ(restored.chat.endpoint, std::string("https://api.example.com/v1"));
    AE_EXPECT_EQ(restored.chat.model, std::string("test-model"));
    AE_EXPECT_EQ(restored.chat.personaPath, std::string("assets/persona/custom.txt"));
    AE_EXPECT_EQ(restored.chat.apiKeySecretName, std::string("custom.key"));
    AE_EXPECT_EQ(restored.chat.maxHistoryMessages, 12);
    AE_EXPECT(restored.chat.temperature > 0.41 && restored.chat.temperature < 0.43);
    AE_EXPECT_EQ(restored.chat.stream, false);
    AE_EXPECT_EQ(restored.chat.timeoutSeconds, 90);
    AE_EXPECT_EQ(restored.chat.enableToolCalls, true);
    AE_EXPECT_EQ(restored.chat.enabled, false);

    AE_EXPECT_EQ(restored.forum.baseUrl, std::string("https://forum.example.com/api"));
    AE_EXPECT_EQ(restored.forum.pageSize, 40);
    AE_EXPECT_EQ(restored.forum.tokenSecretName, std::string("custom.forum"));
    AE_EXPECT_EQ(restored.forum.enabled, false);

    AE_EXPECT_EQ(restored.wall.mode, std::string("hybrid"));
    AE_EXPECT_EQ(restored.wall.nickname, std::string("Snow"));
    AE_EXPECT_EQ(restored.wall.autoRefresh, false);
    AE_EXPECT_EQ(restored.wall.refreshIntervalSeconds, 120);
    AE_EXPECT_EQ(restored.wall.maxLocalMessages, 99);

    AE_EXPECT_EQ(restored.voice.textToSpeechEnabled, true);
    AE_EXPECT_EQ(restored.voice.speechRecognitionEnabled, true);
    AE_EXPECT_EQ(restored.voice.language, std::string("ja-JP"));
    AE_EXPECT_EQ(restored.voice.ttsProvider, std::string("onnx"));
    AE_EXPECT_EQ(restored.voice.sttProvider, std::string("vosk"));
    AE_EXPECT_EQ(restored.voice.voiceId, std::string("voice-7"));
    AE_EXPECT(restored.voice.speechRate > 1.24 && restored.voice.speechRate < 1.26);
    AE_EXPECT(restored.voice.pitch > 0.89 && restored.voice.pitch < 0.91);
    AE_EXPECT_EQ(restored.voice.autoSpeakChatReply, true);
    AE_EXPECT_EQ(restored.voice.pushToTalk, false);
    AE_EXPECT_EQ(restored.voice.wakeWordEnabled, true);
    AE_EXPECT_EQ(restored.voice.localModelPath, std::string("D:/models/vosk"));

    AE_EXPECT_EQ(restored.music.enabled, true);
    AE_EXPECT(restored.music.volume > 0.34 && restored.music.volume < 0.36);
    AE_EXPECT_EQ(restored.music.localFolders.size(), static_cast<std::size_t>(2));
    AE_EXPECT_EQ(restored.music.localFolders[0], std::string("D:/music/a"));
    AE_EXPECT_EQ(restored.music.localFolders[1], std::string("D:/music/b"));
    AE_EXPECT_EQ(restored.music.playMode, std::string("shuffle"));
    AE_EXPECT_EQ(restored.music.onlineProvider, std::string("netease"));
    AE_EXPECT_EQ(restored.music.cacheLyrics, false);
    AE_EXPECT_EQ(restored.music.showLyricBubble, true);

    AE_EXPECT_EQ(restored.update.enabled, false);
    AE_EXPECT_EQ(restored.update.channel, std::string("beta"));
    AE_EXPECT_EQ(restored.update.checkOnStartup, false);
    AE_EXPECT_EQ(restored.update.checkIntervalHours, 6);
    AE_EXPECT_EQ(restored.update.manifestUrl, std::string("https://example.com/update.json"));
    AE_EXPECT_EQ(restored.update.autoDownload, true);
    AE_EXPECT_EQ(restored.update.autoInstall, true);

    AE_EXPECT_EQ(restored.plugins.enabled, true);
    AE_EXPECT_EQ(restored.plugins.pluginDirectories.size(), static_cast<std::size_t>(2));
    AE_EXPECT_EQ(restored.plugins.pluginDirectories[1], std::string("D:/plugins"));
    AE_EXPECT_EQ(restored.plugins.allowNativePlugins, true);
    AE_EXPECT_EQ(restored.plugins.allowNetworkAccess, true);
    AE_EXPECT_EQ(restored.plugins.developerMode, true);

    std::error_code ec;
    std::filesystem::remove_all(directory, ec);
}

AE_TEST(Settings_CorruptFileFallsBackToDefaults) {
    const auto directory = MakeTempDirectory();
    const auto file = directory / "broken.json";
    {
        std::ofstream stream(file, std::ios::binary);
        stream << "{ this is not json";
    }

    Aemeath::Infrastructure::Config::JsonSettingsStore store(file);
    const auto settings = store.Load();
    AE_EXPECT_EQ(settings.pet.topMost, true);
    AE_EXPECT_EQ(settings.chat.provider, std::string("mock"));
    AE_EXPECT_EQ(settings.schemaVersion, 2);

    std::error_code ec;
    std::filesystem::remove_all(directory, ec);
}

AE_TEST(Settings_MissingFileReturnsDefaults) {
    const auto directory = MakeTempDirectory();
    const auto file = directory / "does-not-exist.json";

    Aemeath::Infrastructure::Config::JsonSettingsStore store(file);
    const auto settings = store.Load();

    AE_EXPECT_EQ(settings.pet.topMost, true);
    AE_EXPECT_EQ(settings.chat.provider, std::string("mock"));

    std::error_code ec;
    std::filesystem::remove_all(directory, ec);
}

AE_TEST(UserPaths_LayoutIsLayered) {
    auto& paths = Aemeath::Core::Platform::UserPaths::Instance();
    const auto root = MakeTempDirectory();
    paths.OverrideRoot(root);

    AE_EXPECT_EQ(paths.User(), root / "user");
    AE_EXPECT_EQ(paths.Secrets(), root / "user" / "secrets");
    AE_EXPECT_EQ(paths.State(), root / "user" / "state");
    AE_EXPECT_EQ(paths.Cache(), root / "cache");
    AE_EXPECT_EQ(paths.Logs(), root / "logs");
    AE_EXPECT(std::filesystem::exists(paths.Secrets()));
    AE_EXPECT(std::filesystem::exists(paths.Logs()));
    AE_EXPECT_EQ(paths.SettingsFile(), root / "user" / "settings.json");

    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    paths.ReloadFromEnvironment();
}

} // namespace
