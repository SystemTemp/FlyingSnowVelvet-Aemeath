#pragma once

#include <chrono>
#include <string>

namespace Aemeath::Core::Pet {

enum class PetMood {
    Neutral,
    Happy,
    Sleepy,
    Listening,
    Talking,
    Singing
};

enum class PetActivity {
    Idle,
    Dragging,
    Walking,
    Talking,
    Sleeping,
    Hidden
};

struct PetPosition {
    int x { 100 };
    int y { 100 };
};

struct PetState {
    PetActivity activity { PetActivity::Idle };
    PetMood mood { PetMood::Neutral };
    PetPosition position {};
    double scale { 1.0 };
    bool topMost { true };
    bool clickThrough { false };
    std::string animationId { "idle" };
    std::chrono::system_clock::time_point updatedAt { std::chrono::system_clock::now() };
};

} // namespace Aemeath::Core::Pet
