#include "TestSupport.h"

#include "Aemeath/Core/Pet/AnimationPlayer.h"
#include "Aemeath/Core/Pet/PetDirector.h"

#include <chrono>
#include <string>

namespace {

using Aemeath::Core::Pet::AnimationClip;
using Aemeath::Core::Pet::AnimationLibrary;
using Aemeath::Core::Pet::AnimationPlayer;
using Aemeath::Core::Pet::PetActivity;
using Aemeath::Core::Pet::PetDirector;

AnimationLibrary MakeLibrary() {
    AnimationLibrary library;
    library.petId = "test";

    AnimationClip idle;
    idle.id = "idle";
    idle.fps = 10;
    idle.loop = true;
    idle.frames = { "idle/0.png", "idle/1.png", "idle/2.png" };
    library.clips["idle"] = idle;

    AnimationClip talk;
    talk.id = "talk";
    talk.fps = 5;
    talk.loop = false;
    talk.frames = { "talk/0.png", "talk/1.png" };
    library.clips["talk"] = talk;

    return library;
}

AE_TEST(Animation_PlayAndAdvance) {
    const auto library = MakeLibrary();
    AnimationPlayer player;
    player.Bind(&library);

    AE_EXPECT(player.Play("idle"));
    AE_EXPECT_EQ(std::string(player.CurrentFrame()), std::string("idle/0.png"));

    // fps=10 → 每帧 100ms
    player.Advance(std::chrono::milliseconds(150));
    AE_EXPECT_EQ(std::string(player.CurrentFrame()), std::string("idle/1.png"));

    player.Advance(std::chrono::milliseconds(100));
    AE_EXPECT_EQ(std::string(player.CurrentFrame()), std::string("idle/2.png"));
}

AE_TEST(Animation_LoopWrapsAround) {
    const auto library = MakeLibrary();
    AnimationPlayer player;
    player.Bind(&library);
    player.Play("idle");

    player.Advance(std::chrono::milliseconds(350)); // 3 帧 → 回到第 0 帧
    AE_EXPECT_EQ(std::string(player.CurrentFrame()), std::string("idle/0.png"));
    AE_EXPECT(!player.Finished());
}

AE_TEST(Animation_NonLoopFinishes) {
    const auto library = MakeLibrary();
    AnimationPlayer player;
    player.Bind(&library);

    AE_EXPECT(player.Play("talk"));
    player.Advance(std::chrono::milliseconds(500));
    AE_EXPECT(player.Finished());
    AE_EXPECT_EQ(std::string(player.CurrentFrame()), std::string("talk/1.png"));
}

AE_TEST(Animation_MissingClipReturnsFalse) {
    const auto library = MakeLibrary();
    AnimationPlayer player;
    player.Bind(&library);
    AE_EXPECT(!player.Play("does-not-exist"));
}

AE_TEST(Animation_SameClipDoesNotRestartByDefault) {
    const auto library = MakeLibrary();
    AnimationPlayer player;
    player.Bind(&library);
    player.Play("idle");
    player.Advance(std::chrono::milliseconds(150));
    AE_EXPECT_EQ(player.FrameIndex(), static_cast<std::size_t>(1));

    player.Play("idle"); // 默认不重置
    AE_EXPECT_EQ(player.FrameIndex(), static_cast<std::size_t>(1));

    player.Play("idle", true); // 显式重置
    AE_EXPECT_EQ(player.FrameIndex(), static_cast<std::size_t>(0));
}

AE_TEST(Director_SuppressedWhenBusy) {
    PetDirector director;
    PetDirector::Config config;
    config.idleActionMinSeconds = 0.1;
    config.idleActionMaxSeconds = 0.1;
    config.idleActions = { "wave" };
    config.actionDurationSeconds = 0.2;
    director.Configure(config);

    // 拖拽状态下不应触发任何自动动作。
    const auto suggested = director.Tick(std::chrono::milliseconds(500), PetActivity::Dragging);
    AE_EXPECT(!suggested.has_value());
}

AE_TEST(Director_FiresIdleAction) {
    PetDirector director;
    PetDirector::Config config;
    config.enableIdleActions = true;
    config.idleActionMinSeconds = 0.05;
    config.idleActionMaxSeconds = 0.06;
    config.idleActions = { "wave" };
    config.actionDurationSeconds = 0.1;
    director.Configure(config);

    bool fired = false;
    for (int i = 0; i < 40 && !fired; ++i) {
        const auto suggested = director.Tick(std::chrono::milliseconds(16), PetActivity::Idle);
        if (suggested.has_value() && *suggested == "wave") {
            fired = true;
        }
    }
    AE_EXPECT(fired);
}

} // namespace

