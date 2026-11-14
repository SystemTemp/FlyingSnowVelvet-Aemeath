#include "Aemeath/Core/Events/EventBus.h"
#include "Aemeath/Core/Events/AppEvents.h"
#include "Aemeath/Core/Pet/PetBehaviorStateMachine.h"

#include <cassert>
#include <iostream>

int main() {
    using namespace Aemeath::Core::Events;
    using namespace Aemeath::Core::Pet;

    EventBus bus;
    bool received = false;

    auto subscription = bus.Subscribe<PetPositionChangedEvent>([&](const PetPositionChangedEvent& event) {
        received = (event.position.x == 10 && event.position.y == 20);
    });

    bus.Publish(PetPositionChangedEvent{ PetPosition{10, 20} });
    assert(received);

    PetBehaviorStateMachine machine;
    machine.ToDragging();
    assert(machine.Current().activity == PetActivity::Dragging);
    machine.ToIdle();
    assert(machine.Current().activity == PetActivity::Idle);

    std::cout << "EventBusSmokeTest passed\n";
    return 0;
}
