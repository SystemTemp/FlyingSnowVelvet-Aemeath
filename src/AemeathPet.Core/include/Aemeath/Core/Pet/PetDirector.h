#pragma once

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Aemeath/Core/Pet/PetState.h"

namespace Aemeath::Core::Pet {

/// 行为导演：在 Idle 状态下按随机间隔切换闲时动作，聊天/拖拽期间抑制自动动作。
class PetDirector final {
public:
    struct Config {
        double idleActionMinSeconds { 25.0 };
        double idleActionMaxSeconds { 70.0 };
        bool enableIdleActions { true };
        /// 闲时动作候选（必须存在于资源清单里，否则自动跳过）。
        std::vector<std::string> idleActions { "idle", "wave", "blink" };
        /// 单次闲时动作持续时长（超过后回到 idle）。
        double actionDurationSeconds { 4.0 };
    };

    void Configure(Config config) noexcept;
    [[nodiscard]] const Config& CurrentConfig() const noexcept { return m_config; }

    void Seed(std::uint64_t seed) noexcept;

    /// 用户交互（点击/拖拽/聊天）时调用，重置闲时计时。
    void NotifyInteraction() noexcept;

    /// 每帧推进。返回非空时表示建议切换到的动画 id（调用方决定是否 Play）。
    std::optional<std::string> Tick(std::chrono::steady_clock::duration delta, PetActivity activity);

private:
    double NextIntervalSeconds();
    double NextRandom01();

    Config m_config {};
    double m_idleElapsedSeconds { 0.0 };
    double m_nextActionAtSeconds { 0.0 };
    double m_actionElapsedSeconds { 0.0 };
    bool m_inAction { false };
    bool m_initialised { false };
    std::uint64_t m_state { 0x9E3779B97F4A7C15ull };
};

} // namespace Aemeath::Core::Pet