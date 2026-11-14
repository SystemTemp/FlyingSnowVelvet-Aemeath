#include "Aemeath/Core/Platform/UserPaths.h"

#include <cstdlib>
#include <system_error>

#ifdef _WIN32
#include <windows.h>
#endif

namespace Aemeath::Core::Platform {

namespace {

/// 读环境变量并转成 std::filesystem::path（Windows 走宽字符，避免 cp936 截断）。
std::filesystem::path ReadEnvironmentPath(const char* name) {
#ifdef _WIN32
    std::wstring wide(32, L'\0');
    std::string narrowName(name);
    const int nameLength = ::MultiByteToWideChar(
        CP_UTF8, 0, narrowName.c_str(), static_cast<int>(narrowName.size()), nullptr, 0);
    if (nameLength <= 0) {
        return {};
    }
    wide.assign(static_cast<std::size_t>(nameLength), L'\0');
    ::MultiByteToWideChar(
        CP_UTF8, 0, narrowName.c_str(), static_cast<int>(narrowName.size()), wide.data(), nameLength);

    ::SetLastError(ERROR_SUCCESS);
    const DWORD required = ::GetEnvironmentVariableW(wide.c_str(), nullptr, 0);
    if (required == 0) {
        return {};
    }
    std::wstring value(required, L'\0');
    const DWORD written = ::GetEnvironmentVariableW(wide.c_str(), value.data(), required);
    if (written == 0) {
        return {};
    }
    value.resize(written);
    return std::filesystem::path(value);
#else
    const char* value = std::getenv(name);
    return value ? std::filesystem::path(value) : std::filesystem::path();
#endif
}

std::filesystem::path DefaultRoot() {
    const auto localAppData = ReadEnvironmentPath("LOCALAPPDATA");
    if (!localAppData.empty()) {
        return localAppData / "AemeathDeskPet";
    }
    const auto temp = ReadEnvironmentPath("TEMP");
    if (!temp.empty()) {
        return temp / "AemeathDeskPet";
    }
    return std::filesystem::current_path() / "AemeathDeskPet";
}

} // namespace

UserPaths::UserPaths()
    : m_root(DefaultRoot()) {
    ReloadFromEnvironment();
}

UserPaths& UserPaths::Instance() {
    static UserPaths instance;
    return instance;
}

void UserPaths::ReloadFromEnvironment() {
    const auto override_ = ReadEnvironmentPath("AEMEATH_DESK_PET_HOME");
    m_root = override_.empty() ? DefaultRoot() : override_;
    EnsureCreated();
}

void UserPaths::OverrideRoot(std::filesystem::path root) {
    m_root = std::move(root);
    EnsureCreated();
}

void UserPaths::EnsureCreated() const {
    std::error_code ec;
    std::filesystem::create_directories(Secrets(), ec);
    std::filesystem::create_directories(State(), ec);
    std::filesystem::create_directories(Cache(), ec);
    std::filesystem::create_directories(Logs(), ec);
}

} // namespace Aemeath::Core::Platform