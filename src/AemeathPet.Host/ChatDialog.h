#pragma once

#include "Aemeath/Core/Chat/ChatMessage.h"

#include <windows.h>

#include <functional>
#include <future>
#include <string>

namespace Aemeath::Host {

/// 聊天窗口：多行只读记录 + 输入框 + 发送按钮。回复在后台线程取回，避免卡住 UI。
class ChatDialog final {
public:
    using SendHandler =
        std::function<std::future<Aemeath::Core::Chat::ChatMessage>(std::string prompt)>;
    using SpeakHandler = std::function<void(const std::string& text)>;

    bool Show(HINSTANCE instance, HWND owner, SendHandler sendHandler, SpeakHandler speakHandler);

private:
    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(UINT message, WPARAM wParam, LPARAM lParam);

    void CreateControls();
    void AppendLine(const std::wstring& line);
    void SendCurrent();
    void FinishReply(const std::wstring& reply);
    void RequestClose();

    HWND m_hwnd { nullptr };
    HWND m_owner { nullptr };
    HINSTANCE m_instance { nullptr };
    HWND m_log { nullptr };
    HWND m_input { nullptr };
    HWND m_send { nullptr };
    HWND m_status { nullptr };
    HFONT m_font { nullptr };

    SendHandler m_sendHandler;
    SpeakHandler m_speakHandler;
    bool m_busy { false };

    static constexpr UINT kReplyMessage = WM_APP + 9;
};

} // namespace Aemeath::Host