#include "TestSupport.h"

#include "Aemeath/Core/Events/EventBus.h"
#include "Aemeath/Core/Events/AppEvents.h"

#include <memory>
#include <string>
#include <vector>

using Aemeath::Core::Events::EventBus;
using Aemeath::Core::Events::PetPositionChangedEvent;
using Aemeath::Core::Events::ErrorOccurredEvent;

namespace {

AE_TEST(EventBus_SubscriberReceivesPayload) {
    EventBus bus;
    int received = 0;
    int lastX = 0;

    auto subscription = bus.Subscribe<PetPositionChangedEvent>([&](const PetPositionChangedEvent& event) {
        ++received;
        lastX = event.position.x;
    });

    PetPositionChangedEvent event;
    event.position = { 42, 7 };
    bus.Publish(event);

    AE_EXPECT_EQ(received, 1);
    AE_EXPECT_EQ(lastX, 42);
}

AE_TEST(EventBus_MultipleSubscribersAllNotified) {
    EventBus bus;
    std::vector<std::string> order;

    auto first = bus.Subscribe<ErrorOccurredEvent>([&](const ErrorOccurredEvent&) {
        order.emplace_back("first");
    });
    auto second = bus.Subscribe<ErrorOccurredEvent>([&](const ErrorOccurredEvent&) {
        order.emplace_back("second");
    });

    bus.Publish(ErrorOccurredEvent { "test", "boom" });

    AE_EXPECT_EQ(order.size(), static_cast<std::size_t>(2));
    AE_EXPECT_EQ(order[0], std::string("first"));
    AE_EXPECT_EQ(order[1], std::string("second"));
}

AE_TEST(EventBus_UnsubscribeStopsDelivery) {
    EventBus bus;
    int received = 0;

    {
        auto subscription = bus.Subscribe<ErrorOccurredEvent>([&](const ErrorOccurredEvent&) {
            ++received;
        });
        bus.Publish(ErrorOccurredEvent { "a", "1" });
        AE_EXPECT_EQ(received, 1);
        subscription.Unsubscribe();
    }

    bus.Publish(ErrorOccurredEvent { "a", "2" });
    AE_EXPECT_EQ(received, 1);
}

AE_TEST(EventBus_SubscriptionDiesWithScope) {
    EventBus bus;
    int received = 0;

    {
        auto subscription = bus.Subscribe<ErrorOccurredEvent>([&](const ErrorOccurredEvent&) {
            ++received;
        });
        AE_EXPECT_EQ(received, 0);
    }

    bus.Publish(ErrorOccurredEvent { "a", "3" });
    AE_EXPECT_EQ(received, 0);
}

AE_TEST(EventBus_DifferentTypesDoNotCross) {
    EventBus bus;
    int positions = 0;
    int errors = 0;

    auto a = bus.Subscribe<PetPositionChangedEvent>([&](const PetPositionChangedEvent&) {
        ++positions;
    });
    auto b = bus.Subscribe<ErrorOccurredEvent>([&](const ErrorOccurredEvent&) {
        ++errors;
    });

    bus.Publish(PetPositionChangedEvent {});

    AE_EXPECT_EQ(positions, 1);
    AE_EXPECT_EQ(errors, 0);
}

} // namespace
