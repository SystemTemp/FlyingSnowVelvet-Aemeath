#include "Aemeath/Core/Pet/PetBehaviorStateMachine.h"

namespace Aemeath::Core::Pet {

namespace {
auto Now() {
    return std::chrono::system_clock::now();
}
}

void PetBehaviorStateMachine::SetPosition(PetPosition position) {
    m_state.position = position;
    m_state.updatedAt = Now();
}

void PetBehaviorStateMachine::SetScale(double scale) {
    m_state.scale = scale;
    m_state.updatedAt = Now();
}

void PetBehaviorStateMachine::SetTopMost(bool topMost) {
    m_state.topMost = topMost;
    m_state.updatedAt = Now();
}

void PetBehaviorStateMachine::SetClickThrough(bool clickThrough) {
    m_state.clickThrough = clickThrough;
    m_state.updatedAt = Now();
}

void PetBehaviorStateMachine::ToIdle() {
    m_state.activity = PetActivity::Idle;
    m_state.mood = PetMood::Neutral;
    m_state.animationId = "idle";
    m_state.updatedAt = Now();
}

void PetBehaviorStateMachine::ToDragging() {
    m_state.activity = PetActivity::Dragging;
    m_state.mood = PetMood::Neutral;
    m_state.animationId = "drag";
    m_state.updatedAt = Now();
}

void PetBehaviorStateMachine::ToTalking() {
    m_state.activity = PetActivity::Talking;
    m_state.mood = PetMood::Talking;
    m_state.animationId = "talk";
    m_state.updatedAt = Now();
}

void PetBehaviorStateMachine::Hide() {
    m_state.activity = PetActivity::Hidden;
    m_state.animationId = "hidden";
    m_state.updatedAt = Now();
}

} // namespace Aemeath::Core::Pet
