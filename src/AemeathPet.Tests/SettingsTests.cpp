#include "TestSupport.h"

#include "Aemeath/Core/Config/AppSettings.h"
#include "Aemeath/Core/Platform/UserPaths.h"
#include "Aemeath/Infrastructure/Config/JsonSettingsStore.h"

#include <chrono>
#include <filesystem>
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
    original.pet.x = 1234;
    original.pet.y = 567;
    original.pet.scale = 1.5;
    original.pet.topMost = false;
    original.chat.provider = "openai";
    original.chat.endpoint = "https://api.example.com/v1";
    original.chat.model = "test-model";
    original.app.language = "zh-CN";

    {
        Aemeath::Infrastructure::Config::JsonSettingsStore store(file);
        store.Save(original);
    }

    AE_EXPECT(std::filesystem::exists(file));

    Aemeath::Infrastructure::Config::JsonSettingsStore loaded(file);
    const auto restored = loaded.Load();

    AE_EXPECT_EQ(restored.chat.provider, std::string("openai"));
    AE_EXPECT_EQ(restored.chat.endpoint, std::string("https://api.example.com/v1"));
    AE_EXPECT_EQ(restored.chat.model, std::string("test-model"));

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
