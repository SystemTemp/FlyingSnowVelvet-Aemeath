#include "Aemeath/Core/Pet/AnimationPlayer.h"

#include <algorithm>
#include <utility>

namespace Aemeath::Core::Pet {

const AnimationClip* AnimationLibrary::Find(std::string_view id) const {
    const auto it = clips.find(std::string(id));
    return it == clips.end() ? nullptr : &it->second;
}

bool AnimationPlayer::Play(std::string_view clipId, bool restartIfSame) {
    if (clipId.empty()) {
        return false;
    }
    if (!restartIfSame && m_clipId == clipId) {
        return true;
    }

    m_clipId.assign(clipId);
    m_frame.clear();
    m_frameIndex = 0;
    m_accumulatorSeconds = 0.0;
    m_finished = false;

    if (m_library == nullptr) {
        return false;
    }

    const auto* clip = m_library->Find(clipId);
    if (clip == nullptr || clip->frames.empty()) {
        // 资源里没有这段动画：保留 clipId 以便上层观测，但不算播放中。
        return false;
    }

    m_frame = clip->frames.front();
    return true;
}

std::string_view AnimationPlayer::Advance(std::chrono::steady_clock::duration delta) {
    if (m_clipId.empty() || m_library == nullptr) {
        return m_frame;
    }

    const auto* clip = m_library->Find(m_clipId);
    if (clip == nullptr || clip->frames.size() <= 1) {
        return m_frame;
    }
    if (m_finished) {
        return m_frame;
    }

    const auto fps = clip->fps > 0 ? clip->fps : 8;
    const double frameDuration = 1.0 / static_cast<double>(fps);

    double elapsed = std::chrono::duration<double>(delta).count();
    if (elapsed <= 0.0) {
        return m_frame;
    }
    // 防止长时间挂起后一次跳跃太多帧。
    elapsed = std::min(elapsed, 2.0);
    m_accumulatorSeconds += elapsed;

    while (m_accumulatorSeconds >= frameDuration) {
        m_accumulatorSeconds -= frameDuration;
        if (m_frameIndex + 1 < clip->frames.size()) {
            ++m_frameIndex;
        } else if (clip->loop) {
            m_frameIndex = 0;
        } else {
            m_finished = true;
            break;
        }
        m_frame = clip->frames[m_frameIndex];
    }

    return m_frame;
}

void AnimationPlayer::Stop() {
    m_clipId.clear();
    m_frame.clear();
    m_frameIndex = 0;
    m_accumulatorSeconds = 0.0;
    m_finished = false;
}

} // namespace Aemeath::Core::Pet