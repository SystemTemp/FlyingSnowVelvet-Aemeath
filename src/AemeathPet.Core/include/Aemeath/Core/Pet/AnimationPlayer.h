#pragma once

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Aemeath::Core::Pet {

/// 一段动画：帧序列 + 播放速度。
struct AnimationClip {
    std::string id;
    int fps { 8 };
    bool loop { true };
    std::vector<std::string> frames;
};

/// 宠物资源清单（对应 assets/pet/<id>/manifest.json）。
struct AnimationLibrary {
    std::string petId { "default" };
    std::string displayName;
    double scale { 1.0 };
    std::filesystem::path baseDirectory;
    std::unordered_map<std::string, AnimationClip> clips;

    [[nodiscard]] const AnimationClip* Find(std::string_view id) const;
    [[nodiscard]] bool Empty() const noexcept { return clips.empty(); }
};

/// 帧调度器：与窗口系统解耦，纯时间驱动，便于单元测试。
class AnimationPlayer final {
public:
    void Bind(const AnimationLibrary* library) noexcept { m_library = library; }

    /// 播放指定动画。重复请求同一动画默认不重置进度。
    bool Play(std::string_view clipId, bool restartIfSame = false);

    /// 推进时间，返回当前应显示的帧（相对资源目录）；无动画时返回空串。
    std::string_view Advance(std::chrono::steady_clock::duration delta);

    [[nodiscard]] std::string_view CurrentClipId() const noexcept { return m_clipId; }
    [[nodiscard]] std::string_view CurrentFrame() const noexcept { return m_frame; }
    [[nodiscard]] std::size_t FrameIndex() const noexcept { return m_frameIndex; }
    [[nodiscard]] bool IsPlaying() const noexcept { return !m_clipId.empty(); }

    /// 非循环动画播放结束后为 true。
    [[nodiscard]] bool Finished() const noexcept { return m_finished; }

    void Stop();

private:
    const AnimationLibrary* m_library { nullptr };
    std::string m_clipId;
    std::string m_frame;
    std::size_t m_frameIndex { 0 };
    double m_accumulatorSeconds { 0.0 };
    bool m_finished { false };
};

} // namespace Aemeath::Core::Pet