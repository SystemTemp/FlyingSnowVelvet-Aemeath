#pragma once

#include <windows.h>

#include <string>

namespace Aemeath::Host {

enum class TrayCommand : UINT {
    None = 0,
    Chat = 1001,
    Settings = 1002,
    ToggleClickThrough = 1003,
    ToggleTopMost = 1004,
    ReloadAssets = 1005,
    OpenDataDirectory = 1006,
    Exit = 1007
};

/// 托盘图标 + 托盘菜单（与桌宠右键菜单共用命令 ID）。
class TrayIcon final {
public:
    TrayIcon() = default;
    ~TrayIcon();

    TrayIcon(const TrayIcon&) = delete;
    TrayIcon& operator=(const TrayIcon&) = delete;

    bool Add(HWND owner, UINT callbackMessage, const std::wstring& tooltip);
    void Remove();
    void SetTooltip(const std::wstring& tooltip);
    void SetIcon(HICON icon);

    /// 在屏幕坐标处弹出菜单；返回被点击的命令。
    TrayCommand ShowContextMenu(POINT screenPoint, bool topMost, bool clickThrough);

private:
    HWND m_owner { nullptr };
    UINT m_callbackMessage { 0 };
    HICON m_icon { nullptr };
    bool m_added { false };
};

[[nodiscard]] const wchar_t* CommandLabel(TrayCommand command) noexcept;

} // namespace Aemeath::Host