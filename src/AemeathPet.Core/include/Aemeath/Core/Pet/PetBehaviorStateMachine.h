#pragma once

#include "Aemeath/Core/Pet/PetState.h"

namespace Aemeath::Core::Pet {

class PetBehaviorStateMachine final {
public:
    [[nodiscard]] const PetState& Current() const noexcept { return m_state; }

    void SetPosition(PetPosition position);
    void SetScale(double scale);
    void SetTopMost(bool topMost);
    void SetClickThrough(bool clickThrough);

    void ToIdle();
    void ToDragging();
    void ToTalking();
    void Hide();

private:
    PetState m_state;
};

} // namespace Aemeath::Core::Pet
