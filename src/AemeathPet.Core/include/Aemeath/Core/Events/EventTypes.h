#pragma once

namespace Aemeath::Core::Events {

enum class EventType {
    AppStarted,
    AppStopping,
    PetStateChanged,
    PetPositionChanged,
    PetActionRequested,
    ChatMessageSubmitted,
    ChatMessageReceived,
    SettingsChanged,
    ErrorOccurred
};

} // namespace Aemeath::Core::Events
