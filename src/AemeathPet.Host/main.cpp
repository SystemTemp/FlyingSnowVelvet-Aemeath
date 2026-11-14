#include "ChatDialog.h"
#include "PetHostWindow.h"
#include "SettingsDialog.h"

#include "Aemeath/Application/AppController.h"
#include "Aemeath/Core/Config/AppSettings.h"
#include "Aemeath/Core/Logging/Logger.h"
#include "Aemeath/Core/Platform/UserPaths.h"
#include "Aemeath/Infrastructure/Chat/OpenAICompatibleChatService.h"
#include "Aemeath/Infrastructure/Config/JsonSettingsStore.h"
#include "Aemeath/Infrastructure/Forum/ForumApiClient.h"
#include "Aemeath/Infrastructure/Http/WinHttpClient.h"
#include "Aemeath/Infrastructure/Pet/PetAssetLoader.h"
#include "Aemeath/Services/IChatProvider.h"

#include <windows.h>
#include <shellapi.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace {

using Aemeath::Core::Chat::ChatMessage;
using Aemeath::Core::Chat::ChatRole;
using Aemeath::Core::Chat::ChatSession;
using Aemeath::Core::Config::AppSettings;

constexpr wchar_t kMutexName[] = L"Local\\AemeathPetCpp.SingleInstance";
constexpr wchar_t kWindowTitle[] = L"\u98de\u884c\u96ea\u7ed2 (C++)";
constexpr int kMaxHistory = 24;

std::wstring ToWide(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int size = ::MultiByteToWideChar(
        CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    if (size <= 0) {
        return {};
    }
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    ::MultiByteToWideChar(
        CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), result.data(), size);
    return result;
}

std::string ReadTextFile(const std::filesystem::path& path, std::size_t limit = 64 * 1024) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        return {};
    }
    std::string content;
    content.resize(limit);
    stream.read(content.data(), static_cast<std::streamsize>(limit));
    content.resize(static_cast<std::size_t>(stream.gcount()));
    return content;
}

class Application final {
public:
    Application() = default;
    ~Application();

    int Run(HINSTANCE instance);

private:
    bool Initialize(HINSTANCE instance);
    void Shutdown();

    void HandleCommand(Aemeath::Host::TrayCommand command);
    void OpenChat();
    void OpenSettings();
    void ReloadPetAssets();
    void SaveSettings();

    std::shared_ptr<Aemeath::Services::IChatService> CreateChatService() const;
    Aemeath::Services::ChatProviderConfig BuildProviderConfig() const;
    std::filesystem::path LocateAssets(HINSTANCE instance) const;
    std::string LoadPersona() const;
    std::string LoadApiKey() const;

    HINSTANCE m_instance { nullptr };
    HANDLE m_mutex { nullptr };

    std::shared_ptr<Aemeath::Infrastructure::Config::JsonSettingsStore> m_store;
    std::shared_ptr<Aemeath::Infrastructure::Http::WinHttpClient> m_http;
    std::shared_ptr<Aemeath::Services::IChatService> m_chat;
    std::shared_ptr<Aemeath::Infrastructure::Forum::ForumApiClient> m_forum;

    std::shared_ptr<Aemeath::Core::Pet::AnimationLibrary> m_library;
    std::filesystem::path m_assetsRoot;

    Aemeath::Host::PetHostWindow m_pet;
    ChatSession m_history;
    AppSettings m_settings;
    std::string m_chatProviderInUse { "mock" };
};

Application::~Application() {
    Shutdown();
}

int Application::Run(HINSTANCE instance) {
    m_instance = instance;
    if (!Initialize(instance)) {
        return 1;
    }

    MSG message {};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    SaveSettings();
    Shutdown();
    return static_cast<int>(message.wParam);
}

bool Application::Initialize(HINSTANCE instance) {
    ::SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    const HRESULT comResult = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(comResult) && comResult != RPC_E_CHANGED_MODE) {
        return false;
    }

    auto& paths = Aemeath::Core::Platform::UserPaths::Instance();
    paths.ReloadFromEnvironment();

    auto& logger = Aemeath::Core::Logging::Logger::Instance();
    logger.SetMinimumLevel(Aemeath::Core::Logging::LogLevel::Debug);
    logger.SetLogFile(paths.LogFile());
    logger.Info("app", "==== \u98de\u884c\u96ea\u7ed2 C++ \u7248\u542f\u52a8 ====");
    logger.Info("app", "数据目录：" + paths.Root().string());

    m_mutex = ::CreateMutexW(nullptr, TRUE, kMutexName);
    if (m_mutex == nullptr) {
        logger.Error("app", "创建单实例互斥体失败");
        return false;
    }
    if (::GetLastError() == ERROR_ALREADY_EXISTS) {
        ::MessageBoxW(
            nullptr,
            L"\u98de\u884c\u96ea\u7ed2\u5df2\u7ecf\u5728\u8fd0\u884c\u4e86\u3002",
            kWindowTitle,
            MB_ICONINFORMATION | MB_OK);
        return false;
    }

    m_store = std::make_shared<Aemeath::Infrastructure::Config::JsonSettingsStore>(paths.SettingsFile());
    try {
        m_settings = m_store->Load();
    } catch (const std::exception& ex) {
        logger.Warn("app", std::string("\u8bbe\u7f6e\u52a0\u8f7d\u5931\u8d25\uff0c\u4f7f\u7528\u9ed8\u8ba4\u503c\uff1a") + ex.what());
    }

    m_http = std::make_shared<Aemeath::Infrastructure::Http::WinHttpClient>();
    m_chat = CreateChatService();
    m_forum = std::make_shared<Aemeath::Infrastructure::Forum::ForumApiClient>(
        m_http, m_settings.forum.baseUrl);

    m_assetsRoot = LocateAssets(instance);
    std::string assetError;
    if (auto library = Aemeath::Infrastructure::Pet::PetAssetLoader::LoadPreferred(
            m_assetsRoot, m_settings.pet.characterId, &assetError)) {
        m_library = std::make_shared<Aemeath::Core::Pet::AnimationLibrary>(std::move(*library));
        logger.Info(
            "app",
            "\u52a0\u8f7d\u52a8\u753b\u6e05\u5355\uff1a" + m_library->petId + "\uff0c\u7247\u6bb5 "
                + std::to_string(m_library->clips.size()) + " \u4e2a");
    } else {
        logger.Warn("app", "\u52a8\u753b\u8d44\u6e90\u4e0d\u53ef\u7528\uff1a" + assetError);
    }

    Aemeath::Core::Pet::PetState initial;
    initial.position = { m_settings.pet.x, m_settings.pet.y };
    initial.scale = m_settings.pet.scale;
    initial.topMost = m_settings.pet.topMost;
    initial.clickThrough = m_settings.pet.clickThrough;

    if (!m_pet.Create(instance, initial, L"\u98de\u884c\u96ea\u7ed2")) {
        logger.Error("app", "\u684c\u5ba0\u7a97\u53e3\u521b\u5efa\u5931\u8d25");
        return false;
    }

    if (m_library) {
        m_pet.SetLibrary(m_library);
    }

    m_pet.SetCommandHandler([this](Aemeath::Host::TrayCommand command) {
        HandleCommand(command);
    });

    logger.Info("app", "\u521d\u59cb\u5316\u5b8c\u6210\uff0cAI \u6765\u6e90\uff1a" + m_chatProviderInUse);
    return true;
}

std::shared_ptr<Aemeath::Services::IChatService> Application::CreateChatService() const {
    auto config = BuildProviderConfig();
    return Aemeath::Services::ChatServiceFactory::Create(config, m_http);
}

Aemeath::Services::ChatProviderConfig Application::BuildProviderConfig() const {
    Aemeath::Services::ChatProviderConfig config;
    config.kind = Aemeath::Services::ParseChatProviderKind(m_settings.chat.provider);
    config.endpoint = m_settings.chat.endpoint;
    config.model = m_settings.chat.model;
    config.temperature = m_settings.chat.temperature;
    config.stream = m_settings.chat.stream;
    config.timeoutSeconds = m_settings.chat.timeoutSeconds;
    config.systemPrompt = LoadPersona();
    config.apiKey = LoadApiKey();
    return config;
}

std::string Application::LoadPersona() const {
    if (m_settings.chat.personaPath.empty()) {
        return {};
    }
    std::filesystem::path persona(m_settings.chat.personaPath);
    if (persona.is_relative()) {
        persona = m_assetsRoot / "persona" / persona.filename();
        if (!std::filesystem::exists(persona)) {
            persona = std::filesystem::path(m_settings.chat.personaPath);
        }
    }
    return ReadTextFile(persona);
}

std::string Application::LoadApiKey() const {
    const auto secretFile =
        Aemeath::Core::Platform::UserPaths::Instance().Secrets() / "chat-api-key.txt";
    auto key = ReadTextFile(secretFile, 4096);
    // 去掉行尾空白
    while (!key.empty() && (key.back() == '\r' || key.back() == '\n' || key.back() == ' ')) {
        key.pop_back();
    }
    return key;
}

std::filesystem::path Application::LocateAssets(HINSTANCE instance) const {
    wchar_t exePath[MAX_PATH] {};
    ::GetModuleFileNameW(instance, exePath, MAX_PATH);
    const auto exeDir = std::filesystem::path(exePath).parent_path();

    std::vector<std::filesystem::path> candidates {
        exeDir / "assets",
        exeDir.parent_path() / "assets",
        exeDir / ".." / ".." / "assets",
        exeDir / "share" / "assets",
    };

    for (const auto& candidate : candidates) {
        std::error_code ec;
        if (std::filesystem::exists(candidate / "pet" / "default" / "manifest.json", ec)) {
            return std::filesystem::weakly_canonical(candidate, ec);
        }
    }
    return exeDir / "assets";
}

void Application::ReloadPetAssets() {
    std::string error;
    if (auto library = Aemeath::Infrastructure::Pet::PetAssetLoader::LoadPreferred(
            m_assetsRoot, m_settings.pet.characterId, &error)) {
        m_library = std::make_shared<Aemeath::Core::Pet::AnimationLibrary>(std::move(*library));
        m_pet.SetLibrary(m_library);
        Aemeath::Core::Logging::Logger::Instance().Info("app", "\u52a8\u753b\u8d44\u6e90\u5df2\u91cd\u8f7d");
    } else {
        Aemeath::Core::Logging::Logger::Instance().Warn("app", "\u91cd\u8f7d\u5931\u8d25\uff1a" + error);
    }
}

void Application::OpenChat() {
    if (m_chat == nullptr) {
        return;
    }

    if (m_chatProviderInUse != m_settings.chat.provider) {
        m_chat = CreateChatService();
        m_chatProviderInUse = m_settings.chat.provider;
    }

    Aemeath::Host::ChatDialog dialog;
    const auto send = [this](std::string prompt) {
        ChatMessage user;
        user.id = "user-" + std::to_string(::GetTickCount64());
        user.role = ChatRole::User;
        user.content = std::move(prompt);

        std::vector<ChatMessage> history = m_history.Messages();
        if (history.size() > kMaxHistory) {
            history.erase(history.begin(), history.begin() + (history.size() - kMaxHistory));
        }
        m_history.Add(user);

        auto future = m_chat->SendAsync(std::move(history), user);
        m_pet.BeginTalking();
        return future;
    };

    const auto speak = [this](const std::string& text) {
        (void)this;
        (void)text;
        // TTS 在后续版本接入（WindowsSpeechService 已编译进 Infrastructure）。
    };

    dialog.Show(m_instance, m_pet.Handle(), send, speak);
    m_pet.EndTalking();
}

void Application::OpenSettings() {
    Aemeath::Host::SettingsDialog dialog;
    const bool saved = dialog.Show(
        m_instance,
        nullptr,
        m_settings,
        [this](const AppSettings& updated) {
            const bool topMostChanged = updated.pet.topMost != m_settings.pet.topMost;
            const bool clickThroughChanged = updated.pet.clickThrough != m_settings.pet.clickThrough;
            const bool scaleChanged = updated.pet.scale != m_settings.pet.scale;

            m_settings = updated;
            if (topMostChanged) {
                m_pet.SetTopMost(m_settings.pet.topMost);
            }
            if (clickThroughChanged) {
                m_pet.SetClickThrough(m_settings.pet.clickThrough);
            }
            if (scaleChanged) {
                m_pet.SetScale(m_settings.pet.scale);
            }
            m_chat = CreateChatService();
            m_chatProviderInUse = m_settings.chat.provider;
            SaveSettings();
        });

    if (saved) {
        Aemeath::Core::Logging::Logger::Instance().Info("app", "\u8bbe\u7f6e\u5df2\u4fdd\u5b58");
    }
}

void Application::HandleCommand(Aemeath::Host::TrayCommand command) {
    switch (command) {
    case Aemeath::Host::TrayCommand::Chat:
        OpenChat();
        break;
    case Aemeath::Host::TrayCommand::Settings:
        OpenSettings();
        break;
    case Aemeath::Host::TrayCommand::ToggleClickThrough:
        m_pet.SetClickThrough(!m_pet.IsClickThrough());
        m_settings.pet.clickThrough = m_pet.IsClickThrough();
        SaveSettings();
        break;
    case Aemeath::Host::TrayCommand::ToggleTopMost:
        m_pet.SetTopMost(!m_pet.IsTopMost());
        m_settings.pet.topMost = m_pet.IsTopMost();
        SaveSettings();
        break;
    case Aemeath::Host::TrayCommand::ReloadAssets:
        ReloadPetAssets();
        break;
    case Aemeath::Host::TrayCommand::OpenDataDirectory:
        ::ShellExecuteW(
            nullptr, L"open",
            Aemeath::Core::Platform::UserPaths::Instance().Root().c_str(),
            nullptr, nullptr, SW_SHOWNORMAL);
        break;
    case Aemeath::Host::TrayCommand::Exit:
        if (m_pet.Handle() != nullptr) {
            ::PostMessageW(m_pet.Handle(), WM_CLOSE, 0, 0);
        }
        break;
    case Aemeath::Host::TrayCommand::None:
        break;
    }
}

void Application::SaveSettings() {
    if (m_store == nullptr) {
        return;
    }
    const auto position = m_pet.Position();
    m_settings.pet.x = position.x;
    m_settings.pet.y = position.y;
    m_settings.pet.scale = m_pet.Scale();
    m_settings.pet.topMost = m_pet.IsTopMost();
    m_settings.pet.clickThrough = m_pet.IsClickThrough();

    try {
        m_store->Save(m_settings);
    } catch (const std::exception& ex) {
        Aemeath::Core::Logging::Logger::Instance().Error(
            "app", std::string("\u4fdd\u5b58\u8bbe\u7f6e\u5931\u8d25\uff1a") + ex.what());
    }
}

void Application::Shutdown() {
    Aemeath::Core::Logging::Logger::Instance().Info("app", "==== \u9000\u51fa ====");
    m_pet.Destroy();
    m_library.reset();
    if (m_mutex != nullptr) {
        ::ReleaseMutex(m_mutex);
        ::CloseHandle(m_mutex);
        m_mutex = nullptr;
    }
    ::CoUninitialize();
}

} // namespace

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int) {
    Application app;
    return app.Run(instance);
}