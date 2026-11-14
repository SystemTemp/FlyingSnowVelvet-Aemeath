#include "ChatDialog.h"

#include <windowsx.h>

#include "Aemeath/Core/Logging/Logger.h"

#include <memory>
#include <thread>
#include <utility>

namespace Aemeath::Host {

namespace {

constexpr wchar_t kClassName[] = L"AemeathChatWindow";
constexpr int kIdLog = 3001;
constexpr int kIdInput = 3002;
constexpr int kIdSend = 3003;

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

std::string ToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }
    const int size = ::WideCharToMultiByte(
        CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0) {
        return {};
    }
    std::string result(static_cast<std::size_t>(size), '\0');
    ::WideCharToMultiByte(
        CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), result.data(), size, nullptr, nullptr);
    return result;
}

std::wstring ReadWindowText(HWND control) {
    const int length = GetWindowTextLengthW(control);
    if (length <= 0) {
        return {};
    }
    std::wstring buffer(static_cast<std::size_t>(length) + 1, L'\0');
    GetWindowTextW(control, buffer.data(), length + 1);
    buffer.resize(static_cast<std::size_t>(length));
    return buffer;
}

} // namespace

bool ChatDialog::Show(
    HINSTANCE instance,
    HWND owner,
    SendHandler sendHandler,
    SpeakHandler speakHandler) {
    m_instance = instance;
    m_owner = owner;
    m_sendHandler = std::move(sendHandler);
    m_speakHandler = std::move(speakHandler);
    m_busy = false;

    WNDCLASSEXW windowClass {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = &ChatDialog::StaticWndProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = kClassName;
    if (RegisterClassExW(&windowClass) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    RECT ownerRect { 300, 300, 300, 300 };
    if (owner != nullptr) {
        GetWindowRect(owner, &ownerRect);
    }
    const int width = 460;
    const int height = 560;
    const int x = ownerRect.left + 120;
    const int y = (std::max)(20, static_cast<int>(ownerRect.top) - height / 3);

    m_hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        kClassName,
        L"\u804a\u5929 - \u98de\u884c\u96ea\u7ed2",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_SIZEBOX,
        x, y, width, height,
        owner, nullptr, instance, this);

    if (m_hwnd == nullptr) {
        return false;
    }

    if (owner != nullptr) {
        EnableWindow(owner, FALSE);
    }

    CreateControls();
    AppendLine(L"\u96ea\u7ed2\uff1a\u5728\u5427\u3002\uff08\u5f53\u524d\u6765\u6e90\u53d6\u51b3\u4e8e\u8bbe\u7f6e\u4e2d\u7684 AI \u6765\u6e90\uff09");
    ShowWindow(m_hwnd, SW_SHOW);
    SetFocus(m_input);

    MSG message {};
    while (IsWindow(m_hwnd) && GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (!IsDialogMessageW(m_hwnd, &message)) {
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
    }

    if (owner != nullptr) {
        EnableWindow(owner, TRUE);
        SetActiveWindow(owner);
    }

    if (m_font != nullptr) {
        DeleteObject(m_font);
        m_font = nullptr;
    }
    return true;
}

void ChatDialog::CreateControls() {
    m_font = CreateFontW(
        -15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");

    m_log = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
        10, 10, 424, 430, m_hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdLog)), m_instance, nullptr);
    SendMessageW(m_log, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);

    m_input = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
        10, 452, 340, 28, m_hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdInput)), m_instance, nullptr);
    SendMessageW(m_input, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);

    m_send = CreateWindowExW(
        0, L"BUTTON", L"\u53d1\u9001",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        360, 452, 74, 28, m_hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdSend)), m_instance, nullptr);
    SendMessageW(m_send, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);

    m_status = CreateWindowExW(
        0, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
        10, 488, 424, 22, m_hwnd, nullptr, m_instance, nullptr);
    SendMessageW(m_status, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
    SetWindowTextW(m_status, L"Enter \u53d1\u9001\uff0cShift+Enter \u6362\u884c\u3002");
}

void ChatDialog::AppendLine(const std::wstring& line) {
    const int length = GetWindowTextLengthW(m_log);
    SendMessageW(m_log, EM_SETSEL, static_cast<WPARAM>(length), static_cast<LPARAM>(length));
    std::wstring payload = line;
    payload += L"\r\n";
    SendMessageW(m_log, EM_REPLACESEL, FALSE, reinterpret_cast<LPARAM>(payload.c_str()));
    SendMessageW(m_log, EM_SCROLLCARET, 0, 0);
}

void ChatDialog::SendCurrent() {
    if (m_busy || m_sendHandler == nullptr) {
        return;
    }
    const auto text = ReadWindowText(m_input);
    if (text.empty()) {
        return;
    }

    AppendLine(L"\u4f60\uff1a" + text);
    SetWindowTextW(m_input, L"");
    m_busy = true;
    EnableWindow(m_send, FALSE);
    SetWindowTextW(m_status, L"\u96ea\u7ed2\u6b63\u5728\u601d\u8003...");

    const auto prompt = ToUtf8(text);
    const HWND target = m_hwnd;

    std::thread([this, prompt, target] {
        std::wstring result;
        try {
            auto future = m_sendHandler(prompt);
            const auto reply = future.get();
            result = ToWide(reply.content);
            if (result.empty()) {
                result = L"\uff08\u7a7a\u56de\u590d\uff09";
            }
        } catch (const std::exception& ex) {
            result = L"\u26a0 \u8bf7\u6c42\u5931\u8d25\uff1a" + ToWide(ex.what());
        } catch (...) {
            result = L"\u26a0 \u8bf7\u6c42\u5931\u8d25\uff08\u672a\u77e5\u5f02\u5e38\uff09";
        }

        auto* payload = new std::wstring(std::move(result));
        if (PostMessageW(target, kReplyMessage, 0, reinterpret_cast<LPARAM>(payload)) == FALSE) {
            delete payload;
        }
    }).detach();
}

void ChatDialog::FinishReply(const std::wstring& reply) {
    AppendLine(L"\u96ea\u7ed2\uff1a" + reply);
    m_busy = false;
    EnableWindow(m_send, TRUE);
    SetWindowTextW(m_status, L"Enter \u53d1\u9001\uff0cShift+Enter \u6362\u884c\u3002");

    if (m_speakHandler != nullptr) {
        m_speakHandler(ToUtf8(reply));
    }
}

void ChatDialog::RequestClose() {
    DestroyWindow(m_hwnd);
}

LRESULT CALLBACK ChatDialog::StaticWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto* self = static_cast<ChatDialog*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        if (self != nullptr) {
            self->m_hwnd = hwnd;
        }
    }
    auto* self = reinterpret_cast<ChatDialog*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self == nullptr) {
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    return self->WndProc(message, wParam, lParam);
}

LRESULT ChatDialog::WndProc(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_COMMAND: {
        const int id = LOWORD(wParam);
        if (id == kIdSend) {
            SendCurrent();
            return 0;
        }
        break;
    }

    case kReplyMessage: {
        std::unique_ptr<std::wstring> payload(reinterpret_cast<std::wstring*>(lParam));
        if (payload) {
            FinishReply(*payload);
        }
        return 0;
    }

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            RequestClose();
            return 0;
        }
        break;

    case WM_CLOSE:
        RequestClose();
        return 0;

    default:
        break;
    }

    // 在输入框里按 Enter 直接发送。
    if (message == WM_KEYDOWN && wParam == VK_RETURN) {
        if (GetFocus() == m_input) {
            SendCurrent();
            return 0;
        }
    }

    return DefWindowProcW(m_hwnd, message, wParam, lParam);
}

} // namespace Aemeath::Host
