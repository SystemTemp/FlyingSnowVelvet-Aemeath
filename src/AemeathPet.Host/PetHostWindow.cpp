#include "PetHostWindow.h"

#include "Aemeath/Core/Logging/Logger.h"
#include "Aemeath/Infrastructure/Pet/PetAssetLoader.h"

#include <cmath>
#include <string>

namespace Aemeath::Host {

namespace {

constexpr wchar_t kWindowClass[] = L"AemeathPetHostWindow";
constexpr UINT_PTR kFrameTimerId = 1;
constexpr UINT kFrameIntervalMs = 16;
constexpr int kTalkingHoldMs = 4200;
constexpr int kDefaultSize = 220;

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

} // namespace

PetHostWindow::PetHostWindow() = default;

PetHostWindow::~PetHostWindow() {
    Destroy();
}

bool PetHostWindow::Create(
    HINSTANCE instance,
    const Aemeath::Core::Pet::PetState& initial,
    const std::wstring& tooltip) {
    m_instance = instance;
    m_tooltip = tooltip;
    m_topMost = initial.topMost;
    m_clickThrough = initial.clickThrough;

    WNDCLASSEXW windowClass {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = 0;
    windowClass.lpfnWndProc = &PetHostWindow::StaticWndProc;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    windowClass.hbrBackground = nullptr;
    windowClass.lpszClassName = kWindowClass;

    if (RegisterClassExW(&windowClass) == 0) {
        const DWORD error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS) {
            Aemeath::Core::Logging::Logger::Instance().Error(
                "host", "RegisterClassEx 失败：" + std::to_string(error));
            return false;
        }
    }

    DWORD exStyle = WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
    if (m_topMost) {
        exStyle |= WS_EX_TOPMOST;
    }
    if (m_clickThrough) {
        exStyle |= WS_EX_TRANSPARENT;
    }

    const int width = kDefaultSize;
    const int height = kDefaultSize;

    m_hwnd = CreateWindowExW(
        exStyle,
        kWindowClass,
        L"\u98de\u884c\u96ea\u7ed2",
        WS_POPUP,
        initial.position.x,
        initial.position.y,
        width,
        height,
        nullptr,
        nullptr,
        instance,
        this);

    if (m_hwnd == nullptr) {
        Aemeath::Core::Logging::Logger::Instance().Error(
            "host", "CreateWindowEx 失败：" + std::to_string(GetLastError()));
        return false;
    }

    m_state.SetPosition(initial.position);
    m_state.SetScale(initial.scale);
    m_state.SetTopMost(initial.topMost);
    m_state.SetClickThrough(initial.clickThrough);

    if (!m_renderer.Initialize(m_hwnd)) {
        Aemeath::Core::Logging::Logger::Instance().Error("host", "渲染器初始化失败");
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
        return false;
    }

    ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
    m_trayAdded = m_tray.Add(m_hwnd, kTrayMessage, m_tooltip);
    m_lastTick = GetTickCount64();
    SetTimer(m_hwnd, kFrameTimerId, kFrameIntervalMs, nullptr);

    Aemeath::Core::Logging::Logger::Instance().Info("host", "桌宠窗口已创建");
    return true;
}

void PetHostWindow::Destroy() {
    if (m_hwnd != nullptr) {
        KillTimer(m_hwnd, kFrameTimerId);
        m_tray.Remove();
        m_trayAdded = false;
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    m_renderer.Clear();
}

void PetHostWindow::SetLibrary(std::shared_ptr<const Aemeath::Core::Pet::AnimationLibrary> library) {
    m_library = std::move(library);
    m_lastFrame.clear();
    // 关键：帧调度器必须绑定到新的资源清单，否则所有 Play 都会失败。
    m_player.Bind(m_library.get());

    if (!m_library || m_library->Empty()) {
        Aemeath::Core::Logging::Logger::Instance().Warn("host", "没有任何动画资源可播放");
        return;
    }

    // 让闲时动作只保留资源里真实存在的片段，避免白跑计时。
    Aemeath::Core::Pet::PetDirector::Config directorConfig;
    directorConfig.idleActions.clear();
    for (const auto& [id, clip] : m_library->clips) {
        (void)clip;
        if (id != "idle" && id != "drag" && id != "talk" && id != "hidden") {
            directorConfig.idleActions.push_back(id);
        }
    }
    if (directorConfig.idleActions.empty()) {
        directorConfig.enableIdleActions = false;
    }
    directorConfig.enableIdleActions = directorConfig.enableIdleActions && m_library->Find("idle") != nullptr;
    m_director.Configure(directorConfig);

    PlayClip("idle");
    RenderCurrentFrame(true);
}

void PetHostWindow::PlayClip(const std::string& clipId) {
    if (!m_library) {
        return;
    }
    if (m_player.Play(clipId, true)) {
        return;
    }
    if (clipId != "idle" && m_player.Play("idle", true)) {
        return;
    }
    // 连 idle 都没有：保持当前帧。
    Aemeath::Core::Logging::Logger::Instance().Warn("host", "动画片段不可用：" + clipId);
}

void PetHostWindow::RenderCurrentFrame(bool force) {
    if (!m_library || m_library->Empty()) {
        return;
    }

    std::string frame = std::string(m_player.CurrentFrame());
    if (frame.empty()) {
        if (const auto* idle = m_library->Find("idle"); idle != nullptr && !idle->frames.empty()) {
            frame = idle->frames.front();
        }
    }
    if (frame.empty()) {
        return;
    }

    if (!force && frame == m_lastFrame && m_frameWidth > 0) {
        return;
    }

    const auto path = Aemeath::Infrastructure::Pet::PetAssetLoader::ResolveFrame(*m_library, frame);
    UINT sourceWidth = 0;
    UINT sourceHeight = 0;
    if (!m_renderer.EnsureFrame(path, &sourceWidth, &sourceHeight)) {
        if (frame != m_lastFrame) {
            Aemeath::Core::Logging::Logger::Instance().Warn(
                "render", "帧解码失败：" + path.string());
        }
        m_lastFrame = frame;
        return;
    }

    const double scale = m_state.Current().scale > 0.05 ? m_state.Current().scale : 1.0;
    const int displayWidth = (std::max)(1, static_cast<int>(std::lround(sourceWidth * scale)));
    const int displayHeight = (std::max)(1, static_cast<int>(std::lround(sourceHeight * scale)));

    const auto position = Position();
    if (!m_renderer.RenderFrame(path, displayWidth, displayHeight, position.x, position.y)) {
        return;
    }

    m_lastFrame = frame;
    m_frameWidth = displayWidth;
    m_frameHeight = displayHeight;
}

void PetHostWindow::OnTimer() {
    const ULONGLONG now = GetTickCount64();
    ULONGLONG elapsed = now - m_lastTick;
    m_lastTick = now;
    if (elapsed > 250) {
        // 系统挂起后不要一次补太多帧。
        elapsed = 16;
    }
    const auto delta = std::chrono::milliseconds(elapsed);

    if (m_talking && now - m_talkingSinceMs > kTalkingHoldMs) {
        EndTalking();
    }

    const auto activity = m_state.Current().activity;
    if (const auto suggested = m_director.Tick(delta, activity); suggested.has_value()) {
        PlayClip(*suggested);
    }

    const auto frame = m_player.Advance(delta);
    (void)frame;
    RenderCurrentFrame(false);
}

void PetHostWindow::ShowContextMenu(POINT screenPoint) {
    const auto command = m_tray.ShowContextMenu(screenPoint, m_topMost, m_clickThrough);
    if (command != TrayCommand::None && m_handler) {
        m_handler(command);
    }
}

void PetHostWindow::ApplyAnchoredPosition(int x, int y, bool renderFrame) {
    m_state.SetPosition({ x, y });
    if (renderFrame) {
        RenderCurrentFrame(true);
    } else {
        m_renderer.MoveTo(x, y);
    }
}

Aemeath::Core::Pet::PetPosition PetHostWindow::Position() const noexcept {
    if (m_hwnd == nullptr) {
        return m_state.Current().position;
    }
    RECT rect {};
    if (GetWindowRect(m_hwnd, &rect) == FALSE) {
        return m_state.Current().position;
    }
    return { rect.left, rect.top };
}

void PetHostWindow::SetPosition(int x, int y) {
    m_state.SetPosition({ x, y });
    if (m_frameWidth > 0) {
        m_renderer.MoveTo(x, y);
    }
}

void PetHostWindow::SetScale(double scale) {
    m_state.SetScale(scale);
    RenderCurrentFrame(true);
}

void PetHostWindow::SetTopMost(bool topMost) {
    m_topMost = topMost;
    m_state.SetTopMost(topMost);
    if (m_hwnd != nullptr) {
        SetWindowPos(
            m_hwnd,
            topMost ? HWND_TOPMOST : HWND_NOTOPMOST,
            0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }
}

void PetHostWindow::SetClickThrough(bool clickThrough) {
    m_clickThrough = clickThrough;
    m_state.SetClickThrough(clickThrough);
    if (m_hwnd == nullptr) {
        return;
    }
    const auto style = static_cast<LONG_PTR>(GetWindowLongPtrW(m_hwnd, GWL_EXSTYLE));
    LONG_PTR updated = style;
    if (clickThrough) {
        updated |= WS_EX_TRANSPARENT;
    } else {
        updated &= ~static_cast<LONG_PTR>(WS_EX_TRANSPARENT);
    }
    if (updated != style) {
        SetWindowLongPtrW(m_hwnd, GWL_EXSTYLE, updated);
    }
}

void PetHostWindow::BeginTalking() {
    m_talking = true;
    m_talkingSinceMs = GetTickCount64();
    m_state.ToTalking();
    m_director.NotifyInteraction();
    PlayClip("talk");
}

void PetHostWindow::EndTalking() {
    if (!m_talking) {
        return;
    }
    m_talking = false;
    m_state.ToIdle();
    m_director.NotifyInteraction();
    PlayClip("idle");
}

void PetHostWindow::SetTooltip(const std::wstring& tooltip) {
    m_tooltip = tooltip;
    if (m_trayAdded) {
        m_tray.SetTooltip(tooltip);
    }
}

void PetHostWindow::UpdateLayerSize() {
    RenderCurrentFrame(true);
}

LRESULT CALLBACK PetHostWindow::StaticWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NCCREATE) {
        auto* create = reinterpret_cast<CREATESTRUCTW*>(lParam);
        auto* self = static_cast<PetHostWindow*>(create->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        if (self != nullptr) {
            self->m_hwnd = hwnd;
        }
    }

    auto* self = reinterpret_cast<PetHostWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (self == nullptr) {
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }
    return self->WndProc(message, wParam, lParam);
}

LRESULT PetHostWindow::WndProc(UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_TIMER:
        if (wParam == kFrameTimerId) {
            OnTimer();
            return 0;
        }
        break;

    case WM_LBUTTONDOWN: {
        if (m_clickThrough) {
            return 0;
        }
        m_dragging = true;
        GetCursorPos(&m_dragAnchorCursor);
        m_dragAnchorWindow = Position();
        SetCapture(m_hwnd);
        m_state.ToDragging();
        m_director.NotifyInteraction();
        PlayClip("drag");
        return 0;
    }

    case WM_MOUSEMOVE: {
        if (!m_dragging) {
            return 0;
        }
        POINT cursor {};
        GetCursorPos(&cursor);
        const int x = m_dragAnchorWindow.x + (cursor.x - m_dragAnchorCursor.x);
        const int y = m_dragAnchorWindow.y + (cursor.y - m_dragAnchorCursor.y);
        ApplyAnchoredPosition(x, y, false);
        return 0;
    }

    case WM_LBUTTONUP: {
        if (!m_dragging) {
            return 0;
        }
        m_dragging = false;
        ReleaseCapture();
        m_state.ToIdle();
        m_director.NotifyInteraction();
        PlayClip("idle");
        return 0;
    }

    case WM_RBUTTONUP: {
        POINT cursor {};
        GetCursorPos(&cursor);
        ShowContextMenu(cursor);
        return 0;
    }

    case kTrayMessage: {
        const UINT notify = LOWORD(lParam);
        if (notify == WM_RBUTTONUP || notify == WM_CONTEXTMENU) {
            POINT cursor {};
            GetCursorPos(&cursor);
            ShowContextMenu(cursor);
        } else if (notify == WM_LBUTTONDBLCLK) {
            if (m_handler) {
                m_handler(TrayCommand::Chat);
            }
        }
        return 0;
    }

    case WM_CLOSE:
        DestroyWindow(m_hwnd);
        return 0;

    case WM_DESTROY:
        KillTimer(m_hwnd, kFrameTimerId);
        m_tray.Remove();
        m_trayAdded = false;
        PostQuitMessage(0);
        return 0;

    case WM_DISPLAYCHANGE:
    case WM_DPICHANGED:
        RenderCurrentFrame(true);
        return 0;

    default:
        break;
    }
    return DefWindowProcW(m_hwnd, message, wParam, lParam);
}

} // namespace Aemeath::Host

