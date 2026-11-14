#include "TrayIcon.h"

#include <shellapi.h>
#include <windowsx.h>

#include <cwchar>

#include "Aemeath/Core/Logging/Logger.h"

#pragma warning(push)
#pragma warning(disable : 4996) // NOTIFYICONDATAW 老字段在新 SDK 里仍可用

namespace Aemeath::Host {

namespace {

constexpr UINT kMenuFirst = 1001;
constexpr UINT kMenuLast = 1007;

} // namespace

TrayIcon::~TrayIcon() {
    Remove();
}

bool TrayIcon::Add(HWND owner, UINT callbackMessage, const std::wstring& tooltip) {
    if (m_added) {
        Remove();
    }

    m_owner = owner;
    m_callbackMessage = callbackMessage;
    if (m_icon == nullptr) {
        m_icon = LoadIconW(nullptr, IDI_APPLICATION);
    }

    NOTIFYICONDATAW data {};
    data.cbSize = sizeof(data);
    data.hWnd = owner;
    data.uID = 1;
    data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    data.uCallbackMessage = callbackMessage;
    data.hIcon = m_icon;
    wcsncpy_s(data.szTip, tooltip.c_str(), _TRUNCATE);

    if (!Shell_NotifyIconW(NIM_ADD, &data)) {
        Aemeath::Core::Logging::Logger::Instance().Warn("tray", "Shell_NotifyIcon(NIM_ADD) 失败");
        return false;
    }

    data.uVersion = NOTIFYICON_VERSION_4;
    Shell_NotifyIconW(NIM_SETVERSION, &data);
    m_added = true;
    return true;
}

void TrayIcon::Remove() {
    if (!m_added) {
        return;
    }
    NOTIFYICONDATAW data {};
    data.cbSize = sizeof(data);
    data.hWnd = m_owner;
    data.uID = 1;
    Shell_NotifyIconW(NIM_DELETE, &data);
    m_added = false;
}

void TrayIcon::SetTooltip(const std::wstring& tooltip) {
    if (!m_added) {
        return;
    }
    NOTIFYICONDATAW data {};
    data.cbSize = sizeof(data);
    data.hWnd = m_owner;
    data.uID = 1;
    data.uFlags = NIF_TIP;
    wcsncpy_s(data.szTip, tooltip.c_str(), _TRUNCATE);
    Shell_NotifyIconW(NIM_MODIFY, &data);
}

void TrayIcon::SetIcon(HICON icon) {
    m_icon = icon;
    if (!m_added) {
        return;
    }
    NOTIFYICONDATAW data {};
    data.cbSize = sizeof(data);
    data.hWnd = m_owner;
    data.uID = 1;
    data.uFlags = NIF_ICON;
    data.hIcon = icon;
    Shell_NotifyIconW(NIM_MODIFY, &data);
}

TrayCommand TrayIcon::ShowContextMenu(POINT screenPoint, bool topMost, bool clickThrough) {
    HMENU menu = CreatePopupMenu();
    if (menu == nullptr) {
        return TrayCommand::None;
    }

    AppendMenuW(menu, MF_STRING, static_cast<UINT>(TrayCommand::Chat), L"聊天");
    AppendMenuW(menu, MF_STRING, static_cast<UINT>(TrayCommand::Settings), L"设置");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(
        menu,
        MF_STRING | (clickThrough ? MF_CHECKED : MF_UNCHECKED),
        static_cast<UINT>(TrayCommand::ToggleClickThrough),
        L"点击穿透");
    AppendMenuW(
        menu,
        MF_STRING | (topMost ? MF_CHECKED : MF_UNCHECKED),
        static_cast<UINT>(TrayCommand::ToggleTopMost),
        L"窗口置顶");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, static_cast<UINT>(TrayCommand::ReloadAssets), L"重载动画资源");
    AppendMenuW(menu, MF_STRING, static_cast<UINT>(TrayCommand::OpenDataDirectory), L"打开数据目录");
    AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(menu, MF_STRING, static_cast<UINT>(TrayCommand::Exit), L"退出");

    SetForegroundWindow(m_owner);
    const UINT chosen = TrackPopupMenu(
        menu,
        TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
        screenPoint.x,
        screenPoint.y,
        0,
        m_owner,
        nullptr);
    DestroyMenu(menu);

    if (chosen < kMenuFirst || chosen > kMenuLast) {
        return TrayCommand::None;
    }
    return static_cast<TrayCommand>(chosen);
}

const wchar_t* CommandLabel(TrayCommand command) noexcept {
    switch (command) {
    case TrayCommand::Chat: return L"聊天";
    case TrayCommand::Settings: return L"设置";
    case TrayCommand::ToggleClickThrough: return L"点击穿透";
    case TrayCommand::ToggleTopMost: return L"窗口置顶";
    case TrayCommand::ReloadAssets: return L"重载动画资源";
    case TrayCommand::OpenDataDirectory: return L"打开数据目录";
    case TrayCommand::Exit: return L"退出";
    case TrayCommand::None: break;
    }
    return L"";
}

} // namespace Aemeath::Host

#pragma warning(pop)
