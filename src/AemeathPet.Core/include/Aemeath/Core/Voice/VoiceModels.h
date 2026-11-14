#pragma once

#include <string>

namespace Aemeath::Core::Voice {

struct VoiceProfile {
    std::string id;
    std::string displayName;
    std::string language;
    std::string gender;
};

struct SpeechRecognitionResult {
    std::string text;
    double confidence { 0.0 };
    bool isFinal { true };
};

} // namespace Aemeath::Core::Voice
