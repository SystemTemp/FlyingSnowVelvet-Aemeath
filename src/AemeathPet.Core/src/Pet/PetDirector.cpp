#include "Aemeath/Core/Pet/PetDirector.h"

#include <algorithm>
#include <cmath>

namespace Aemeath::Core::Pet {

void PetDirector::Configure(Config config) noexcept {
    m_config = std::move(config);
    m_initialised = false;
    m_idleElapsedSeconds = 0.0;
    m_actionElapsedSeconds = 0.0;
    m_inAction = false;
}

void PetDirector::Seed(std::uint64_t seed) noexcept {
    m_state = seed != 0 ? seed : 0x9E3779B97F4A7C15ull;
    m_initialised = false;
}

void PetDirector::NotifyInteraction() noexcept {
    m_idleElapsedSeconds = 0.0;
    m_actionElapsedSeconds = 0.0;
    m_inAction = false;
    m_initialised = false;
}

double PetDirector::NextRandom01() {
    // xorshift64*：够用、无外部依赖、可选种子便于测试复现。
    m_state ^= m_state >> 12;
    m_state ^= m_state << 25;
    m_state ^= m_state >> 27;
    const std::uint64_t value = m_state * 0x2545F4914F6CDD1Dull;
    return static_cast<double>(value >> 11) / static_cast<double>(1ull << 53);
}

double PetDirector::NextIntervalSeconds() {
    const double minSeconds = (std::max)(0.01, m_config.idleActionMinSeconds);
    const double maxSeconds = (std::max)(minSeconds, m_config.idleActionMaxSeconds);
    return minSeconds + (maxSeconds - minSeconds) * NextRandom01();
}

std::optional<std::string> PetDirector::Tick(std::chrono::steady_clock::duration delta, PetActivity activity) {
    // 只在 Idle 下产生自动动作；拖拽/说话/睡眠/隐藏一律抑制。
    if (activity != PetActivity::Idle || !m_config.enableIdleActions || m_config.idleActions.empty()) {
        m_idleElapsedSeconds = 0.0;
        m_actionElapsedSeconds = 0.0;
        m_inAction = false;
        m_initialised = false;
        return std::nullopt;
    }

    const double seconds = std::chrono::duration<double>(delta).count();
    if (seconds <= 0.0) {
        return std::nullopt;
    }

    if (!m_initialised) {
        m_initialised = true;
        m_nextActionAtSeconds = NextIntervalSeconds();
        m_idleElapsedSeconds = 0.0;
    }

    if (m_inAction) {
        m_actionElapsedSeconds += seconds;
        if (m_actionElapsedSeconds >= std::max(0.1, m_config.actionDurationSeconds)) {
            m_inAction = false;
            m_actionElapsedSeconds = 0.0;
            m_idleElapsedSeconds = 0.0;
            m_nextActionAtSeconds = NextIntervalSeconds();
            return std::string("idle");
        }
        return std::nullopt;
    }

    m_idleElapsedSeconds += seconds;
    if (m_idleElapsedSeconds < m_nextActionAtSeconds) {
        return std::nullopt;
    }

    // 从候选里挑一个非 idle 的动作；全是 idle 时不打扰用户。
    std::vector<std::string> candidates;
    candidates.reserve(m_config.idleActions.size());
    for (const auto& action : m_config.idleActions) {
        if (!action.empty() && action != "idle") {
            candidates.push_back(action);
        }
    }
    if (candidates.empty()) {
        m_idleElapsedSeconds = 0.0;
        m_nextActionAtSeconds = NextIntervalSeconds();
        return std::nullopt;
    }

    const auto index = static_cast<std::size_t>(NextRandom01() * static_cast<double>(candidates.size()));
    const std::string chosen = candidates[std::min(index, candidates.size() - 1)];

    m_inAction = true;
    m_actionElapsedSeconds = 0.0;
    m_idleElapsedSeconds = 0.0;
    return chosen;
}

} // namespace Aemeath::Core::Pet
