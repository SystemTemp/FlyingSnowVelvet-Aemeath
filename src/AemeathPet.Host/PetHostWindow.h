#pragma once

#include "Aemeath/Core/Pet/AnimationPlayer.h"
#include "Aemeath/Core/Pet/PetBehaviorStateMachine.h"
#include "Aemeath/Core/Pet/PetDirector.h"
#include "PetRenderer.h"
#include "TrayIcon.h"

#include <windows.h>

#include <functional>
#include <memory>
#include <string>

namespace Aemeath::Host {

/// 桌宠主窗口：分层透明 + 置顶 + 工具窗口 + 可拖拽 + 托盘菜单。
class PetHostWindow final {
public:
    using CommandHandler = std::function<void(TrayCommand)>;

    PetHostWindow();
    ~PetHostWindow();

    PetHostWindow(const PetHostWindow&) = delete;
    PetHostWindow& operator=(const PetHostWindow&) = delete;

    bool Create(HINSTANCE instance, const Aemeath::Core::Pet::PetState& initial, const std::wstring& tooltip);
    void Destroy();

    /// 资源清单由调用方持有；本窗口只保留弱生命周期引用（shared_ptr 便于热重载）。
    void SetLibrary(std::shared_ptr<const Aemeath::Core::Pet::AnimationLibrary> library);
    void SetCommandHandler(CommandHandler handler) { m_handler = std::move(handler); }

    void SetScale(double scale);
    void SetTopMost(bool topMost);
    void SetClickThrough(bool clickThrough);
    [[nodiscard]] bool IsClickThrough() const noexcept { return m_clickThrough; }
    [[nodiscard]] bool IsTopMost() const noexcept { return m_topMost; }
    [[nodiscard]] double Scale() const noexcept { return m_state.Current().scale; }

    [[nodiscard]] Aemeath::Core::Pet::PetPosition Position() const noexcept;
    void SetPosition(int x, int y);

    /// 聊天开始/结束：进入 Talking 动画，持续到 EndTalking。
    void BeginTalking();
    void EndTalking();

    void SetTooltip(const std::wstring& tooltip);
    [[nodiscard]] HWND Handle() const noexcept { return m_hwnd; }
    explicit operator bool() const noexcept { return m_hwnd != nullptr; }

    static constexpr UINT kTrayMessage = WM_APP + 1;

private:
    static LRESULT CALLBACK StaticWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT WndProc(UINT message, WPARAM wParam, LPARAM lParam);

    void OnTimer();
    void RenderCurrentFrame(bool force);
    void PlayClip(const std::string& clipId);
    void ShowContextMenu(POINT screenPoint);
    void ApplyAnchoredPosition(int x, int y, bool renderFrame);
    void UpdateLayerSize();

    HWND m_hwnd { nullptr };
    HINSTANCE m_instance { nullptr };
    PetRenderer m_renderer;
    TrayIcon m_tray;
    Aemeath::Core::Pet::PetBehaviorStateMachine m_state;
    Aemeath::Core::Pet::AnimationPlayer m_player;
    Aemeath::Core::Pet::PetDirector m_director;
    std::shared_ptr<const Aemeath::Core::Pet::AnimationLibrary> m_library;

    CommandHandler m_handler;
    std::wstring m_tooltip;

    std::string m_lastFrame;
    int m_frameWidth { 0 };
    int m_frameHeight { 0 };

    bool m_topMost { true };
    bool m_clickThrough { false };
    bool m_dragging { false };
    bool m_talking { false };
    bool m_trayAdded { false };
    ULONGLONG m_talkingSinceMs { 0 };

    POINT m_dragAnchorCursor { 0, 0 };
    Aemeath::Core::Pet::PetPosition m_dragAnchorWindow {};

    ULONGLONG m_lastTick {};
};

} // namespace Aemeath::Host