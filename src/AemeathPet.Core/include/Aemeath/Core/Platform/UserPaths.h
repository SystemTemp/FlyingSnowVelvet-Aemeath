#pragma once

#include <filesystem>
#include <string>

namespace Aemeath::Core::Platform {

/// 运行期数据目录分层（与 Python 原版 C:\AemeathDeskPet 布局保持一致）。
///
/// root/
///   user/settings.json      用户稀疏设置
///   user/secrets/           API Key、Cookie、登录态
///   user/state/             聊天记录、音乐历史、游戏统计
///   cache/                  可再生成缓存
///   logs/                   日志
class UserPaths final {
public:
    static UserPaths& Instance();

    /// AEMEATH_DESK_PET_HOME 优先；否则默认 %LOCALAPPDATA%\AemeathDeskPet。
    void ReloadFromEnvironment();

    /// 测试用：显式覆盖根目录。
    void OverrideRoot(std::filesystem::path root);

    [[nodiscard]] const std::filesystem::path& Root() const noexcept { return m_root; }
    [[nodiscard]] std::filesystem::path User() const { return m_root / "user"; }
    [[nodiscard]] std::filesystem::path Secrets() const { return m_root / "user" / "secrets"; }
    [[nodiscard]] std::filesystem::path State() const { return m_root / "user" / "state"; }
    [[nodiscard]] std::filesystem::path Cache() const { return m_root / "cache"; }
    [[nodiscard]] std::filesystem::path Logs() const { return m_root / "logs"; }

    [[nodiscard]] std::filesystem::path SettingsFile() const { return User() / "settings.json"; }
    [[nodiscard]] std::filesystem::path ForumSessionFile() const { return State() / "forum-session.json"; }
    [[nodiscard]] std::filesystem::path ChatHistoryFile() const { return State() / "chat-history.json"; }
    [[nodiscard]] std::filesystem::path MusicHistoryFile() const { return State() / "music-history.json"; }
    [[nodiscard]] std::filesystem::path LogFile() const { return Logs() / "aemeath.log"; }

    /// 创建全部子目录（幂等）。
    void EnsureCreated() const;

private:
    UserPaths();
    std::filesystem::path m_root;
};

} // namespace Aemeath::Core::Platform