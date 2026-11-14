#pragma once

#include "Aemeath/Core/Config/AppSettings.h"

#include <windows.h>

#include <functional>
#include <string>

namespace Aemeath::Host {

/// 设置窗口（自建 Win32 控件，不依赖任何 UI 框架）。
class SettingsDialog final {
public:
    using SaveHandler = std::function<void(const Aemeath::Core::Config::AppSettings&)>;

    /// 以模态方式打开；返回 true 表示用户点击了保存（onSave 已被调用）。
    bool Show(
        HINSTANCE instance,
        HWND owner,
        const Aemeath::Core::Config::AppSettings& settings,
        SaveHandler onSave);

private:
    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(UINT message, WPARAM wParam, LPARAM lParam);

    void CreateControls();
    void LoadValues();
    void SaveValues();

    HWND m_hwnd { nullptr };
    HWND m_owner { nullptr };
    HINSTANCE m_instance { nullptr };
    Aemeath::Core::Config::AppSettings m_settings;
    SaveHandler m_onSave;

    HWND m_topMost { nullptr };
    HWND m_clickThrough { nullptr };
    HWND m_scale { nullptr };
    HWND m_startMinimized { nullptr };
    HWND m_provider { nullptr };
    HWND m_endpoint { nullptr };
    HWND m_apiKey { nullptr };
    HWND m_model { nullptr };
    HWND m_persona { nullptr };
    HWND m_tts { nullptr };
    HWND m_autoSpeak { nullptr };
    HWND m_status { nullptr };
    HWND m_save { nullptr };
    HWND m_cancel { nullptr };

    HFONT m_font { nullptr };
    bool m_saved { false };
};

} // namespace Aemeath::Host