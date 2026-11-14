#pragma once

#include "Aemeath/Core/Voice/VoiceModels.h"
#include <functional>
#include <future>
#include <string>
#include <vector>

namespace Aemeath::Services {

class ITextToSpeechService {
public:
    virtual ~ITextToSpeechService() = default;
    virtual std::vector<Aemeath::Core::Voice::VoiceProfile> ListVoices() = 0;
    virtual std::future<void> SpeakAsync(std::string text, Aemeath::Core::Voice::VoiceProfile voice) = 0;
    virtual void Stop() = 0;
};

class ISpeechRecognitionService {
public:
    virtual ~ISpeechRecognitionService() = default;
    virtual void Start(
        std::function<void(Aemeath::Core::Voice::SpeechRecognitionResult)> onResult,
        std::function<void(std::string)> onError) = 0;
    virtual void Stop() = 0;
};

} // namespace Aemeath::Services
