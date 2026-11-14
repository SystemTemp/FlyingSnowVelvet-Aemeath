#pragma once

#include "Aemeath/Core/Pet/PetState.h"
#include "Aemeath/Core/Chat/ChatMessage.h"
#include <string>

namespace Aemeath::Core::Events {

struct AppStartedEvent {};
struct AppStoppingEvent {};

struct PetStateChangedEvent {
    Aemeath::Core::Pet::PetState previous;
    Aemeath::Core::Pet::PetState current;
};

struct PetPositionChangedEvent {
    Aemeath::Core::Pet::PetPosition position;
};

struct ChatMessageSubmittedEvent {
    Aemeath::Core::Chat::ChatMessage message;
};

struct ChatMessageReceivedEvent {
    Aemeath::Core::Chat::ChatMessage message;
};

struct SettingsChangedEvent {
    std::string key;
};

struct ErrorOccurredEvent {
    std::string source;
    std::string message;
};

} // namespace Aemeath::Core::Events
