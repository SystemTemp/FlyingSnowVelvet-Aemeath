#include "SettingsDialog.h"

#include <windowsx.h>

#include "Aemeath/Core/Logging/Logger.h"

#include <algorithm>
#include <string>

namespace Aemeath::Host {

namespace {

constexpr wchar_t kClassName[] = L"AemeathSettingsWindow";

constexpr int kIdTopMost = 2001;
constexpr int kIdClickThrough = 2002;
constexpr int kIdScale = 2003;
constexpr int kIdStartMinimized = 2004;
constexpr int kIdProvider = 2005;
constexpr int kIdEndpoint = 2006;
constexpr int kIdApiKey = 2007;
constexpr int kIdModel = 2008;
constexpr int kIdPersona = 2009;
constexpr int kIdTts = 2010;
constexpr int kIdAutoSpeak = 2011;
constexpr int kIdSave = 2012;
constexpr int kIdCancel = 2013;

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

void AddLabel(HWND parent, HFONT font, const wchar_t* text, int x, int y, int width) {
    HWND label = CreateWindowExW(
        0, L"STATIC", text, WS_CHILD | WS_VISIBLE,
        x, y, width, 20, parent, nullptr, nullptr, nullptr);
    SendMessageW(label, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

} // namespace

bool SettingsDialog::Show(
    HINSTANCE instance,
    HWND owner,
    const Aemeath::Core::Config::AppSettings& settings,
    SaveHandler onSave) {
    m_instance = instance;
    m_owner = owner;
    m_settings = settings;
    m_onSave = std::move(onSave);
    m_saved = false;

    WNDCLASSEXW windowClass {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = &SettingsDialog::StaticWndProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = kClassName;
    if (RegisterClassExW(&windowClass) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    const int width = 560;
    const int height = 520;
    RECT ownerRect {};
    if (owner != nullptr && GetWindowRect(owner, &ownerRect)) {
        // 居中到桌宠附近
    } else {
        SetRect(&ownerRect, 200, 200, 200, 200);
    }
    const int x = ownerRect.left + 40;
    const int y = (std::max)(20, static_cast<int>(ownerRect.top) - height / 2);

    m_hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        kClassName,
        L"\u8bbe\u7f6e - \u98de\u884c\u96ea\u7ed2",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        x, y, width, height,
        owner, nullptr, instance, this);

    if (m_hwnd == nullptr) {
        return false;
    }

    if (owner != nullptr) {
        EnableWindow(owner, FALSE);
    }

    CreateControls();
    LoadValues();
    ShowWindow(m_hwnd, SW_SHOW);
    UpdateWindow(m_hwnd);

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
    return m_saved;
}

void SettingsDialog::CreateControls() {
    m_font = CreateFontW(
        -15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");

    auto makeButton = [this](const wchar_t* text, int id, int x, int y, int w, int h, DWORD style) {
        HWND control = CreateWindowExW(
            0, L"BUTTON", text, WS_CHILD | WS_VISIBLE | style,
            x, y, w, h, m_hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), m_instance, nullptr);
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
        return control;
    };

    auto makeEdit = [this](int id, int x, int y, int w) {
        HWND control = CreateWindowExW(
            WS_EX_CLIENTEDGE, L"EDIT", L"",
            WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
            x, y, w, 24, m_hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)), m_instance, nullptr);
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
        return control;
    };

    m_topMost = makeButton(L"\u7a97\u53e3\u7f6e\u9876", kIdTopMost, 20, 15, 150, 24, BS_AUTOCHECKBOX);
    m_clickThrough = makeButton(L"\u70b9\u51fb\u7a7f\u900f", kIdClickThrough, 20, 45, 150, 24, BS_AUTOCHECKBOX);
    m_startMinimized = makeButton(L"\u542f\u52a8\u65f6\u9690\u85cf\u4e3b\u7a97\u53e3", kIdStartMinimized, 20, 75, 220, 24, BS_AUTOCHECKBOX);

    AddLabel(m_hwnd, m_font, L"\u7f29\u653e (0.3 - 3.0)", 20, 108, 160);
    m_scale = makeEdit(kIdScale, 190, 103, 80);

    AddLabel(m_hwnd, m_font, L"AI \u6765\u6e90", 20, 143, 160);
    m_provider = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"COMBOBOX", L"",
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
        190, 138, 320, 200, m_hwnd,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kIdProvider)), m_instance, nullptr);
    SendMessageW(m_provider, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);
    for (const wchar_t* item : { L"mock (\u672c\u5730\u56de\u590d)", L"openai (\u517c\u5bb9 API)",
                                 L"ollama (\u672c\u5730\u6a21\u578b)", L"quick-welfare (\u798f\u5229\u6c60)",
                                 L"backup-welfare (\u5907\u7528\u798f\u5229)" }) {
        SendMessageW(m_provider, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item));
    }

    AddLabel(m_hwnd, m_font, L"Endpoint", 20, 178, 160);
    m_endpoint = makeEdit(kIdEndpoint, 190, 173, 320);

    AddLabel(m_hwnd, m_font, L"API Key", 20, 213, 160);
    m_apiKey = makeEdit(kIdApiKey, 190, 208, 320);

    AddLabel(m_hwnd, m_font, L"\u6a21\u578b", 20, 248, 160);
    m_model = makeEdit(kIdModel, 190, 243, 320);

    AddLabel(m_hwnd, m_font, L"\u4eba\u683c\u6587\u4ef6", 20, 283, 160);
    m_persona = makeEdit(kIdPersona, 190, 278, 320);

    m_tts = makeButton(L"\u542f\u7528\u8bed\u97f3\u64ad\u62a5 (SAPI)", kIdTts, 20, 315, 240, 24, BS_AUTOCHECKBOX);
    m_autoSpeak = makeButton(
        L"\u804a\u5929\u56de\u590d\u81ea\u52a8\u6717\u8bfb", kIdAutoSpeak, 20, 345, 240, 24, BS_AUTOCHECKBOX);

    m_status = CreateWindowExW(
        0, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
        20, 380, 500, 40, m_hwnd, nullptr, m_instance, nullptr);
    SendMessageW(m_status, WM_SETFONT, reinterpret_cast<WPARAM>(m_font), TRUE);

    m_save = makeButton(L"\u4fdd\u5b58", kIdSave, 320, 430, 100, 30, BS_DEFPUSHBUTTON);
    m_cancel = makeButton(L"\u53d6\u6d88", kIdCancel, 430, 430, 100, 30, BS_PUSHBUTTON);
}

void SettingsDialog::LoadValues() {
    Button_SetCheck(m_topMost, m_settings.pet.topMost ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(m_clickThrough, m_settings.pet.clickThrough ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(m_startMinimized, m_settings.app.startMinimized ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(m_tts, m_settings.voice.textToSpeechEnabled ? BST_CHECKED : BST_UNCHECKED);
    Button_SetCheck(m_autoSpeak, m_settings.voice.autoSpeakChatReply ? BST_CHECKED : BST_UNCHECKED);

    SetWindowTextW(m_scale, ToWide(std::to_string(m_settings.pet.scale)).c_str());

    const auto provider = ToWide(m_settings.chat.provider);
    const int index = static_cast<int>(SendMessageW(
        m_provider, CB_FINDSTRINGEXACT, static_cast<WPARAM>(-1),
        reinterpret_cast<LPARAM>(provider.c_str())));
    SendMessageW(m_provider, CB_SETCURSEL, index >= 0 ? index : 0, 0);

    SetWindowTextW(m_endpoint, ToWide(m_settings.chat.endpoint).c_str());
    SetWindowTextW(m_model, ToWide(m_settings.chat.model).c_str());
    SetWindowTextW(m_persona, ToWide(m_settings.chat.personaPath).c_str());
    SetWindowTextW(m_apiKey, L"");
    SetWindowTextW(
        m_status,
        L"\u63d0\u793a\uff1aAPI Key \u4e0d\u5199\u5165 settings.json\uff0c\u800c\u662f\u4fdd\u5b58\u5230 user/secrets \u76ee\u5f55\u3002");
}

void SettingsDialog::SaveValues() {
    m_settings.pet.topMost = Button_GetCheck(m_topMost) == BST_CHECKED;
    m_settings.pet.clickThrough = Button_GetCheck(m_clickThrough) == BST_CHECKED;
    m_settings.app.startMinimized = Button_GetCheck(m_startMinimized) == BST_CHECKED;
    m_settings.voice.textToSpeechEnabled = Button_GetCheck(m_tts) == BST_CHECKED;
    m_settings.voice.autoSpeakChatReply = Button_GetCheck(m_autoSpeak) == BST_CHECKED;

    try {
        const auto scaleText = ToUtf8(ReadWindowText(m_scale));
        if (!scaleText.empty()) {
            const double scale = std::stod(scaleText);
            if (scale >= 0.3 && scale <= 3.0) {
                m_settings.pet.scale = scale;
            }
        }
    } catch (const std::exception&) {
        Aemeath::Core::Logging::Logger::Instance().Warn("settings", "缩放值不是合法数字，已忽略");
    }

    const int providerIndex = static_cast<int>(SendMessageW(m_provider, CB_GETCURSEL, 0, 0));
    switch (providerIndex) {
    case 1: m_settings.chat.provider = "openai"; break;
    case 2: m_settings.chat.provider = "ollama"; break;
    case 3: m_settings.chat.provider = "quick-welfare"; break;
    case 4: m_settings.chat.provider = "backup-welfare"; break;
    default: m_settings.chat.provider = "mock"; break;
    }

    m_settings.chat.endpoint = ToUtf8(ReadWindowText(m_endpoint));
    m_settings.chat.model = ToUtf8(ReadWindowText(m_model));
    m_settings.chat.personaPath = ToUtf8(ReadWindowText(m_persona));

    const auto apiKey = ToUtf8(ReadWindowText(m_apiKey));
    if (!apiKey.empty()) {
        m_settings.chat.apiKeySecretName = "aemeath.chat.apiKey";
    }

    m_saved = true;
    if (m_onSave) {
        m_onSave(m_settings);
    }
}

LRESULT CALLBACK SettingsDialog::StaticWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto* self = static_cast<SettingsDialog*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        if (self != nullptr) {
            self->m_hwnd = hwnd;
        }
    }
    auto* self = reinterpret_cast<SettingsDialog*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self == nullptr) {
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    return self->WndProc(message, wParam, lParam);
}

LRESULT SettingsDialog::WndProc(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_COMMAND: {
        const int id = LOWORD(wParam);
        if (id == kIdSave) {
            SaveValues();
            DestroyWindow(m_hwnd);
            return 0;
        }
        if (id == kIdCancel) {
            m_saved = false;
            DestroyWindow(m_hwnd);
            return 0;
        }
        break;
    }
    case WM_CLOSE:
        m_saved = false;
        DestroyWindow(m_hwnd);
        return 0;
    default:
        break;
    }
    return DefWindowProcW(m_hwnd, message, wParam, lParam);
}

} // namespace Aemeath::Host
