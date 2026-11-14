#pragma once
#include "Aemeath/Services/IMusicServices.h"
#include "Aemeath/Services/IVoiceServices.h"
#include <atomic>
namespace Aemeath::Infrastructure::Media {class NullMusicPlayer final:public Aemeath::Services::IMusicPlayerService{public:void Play(const Aemeath::Core::Music::Track&)override;void Pause()override;void Resume()override;void Stop()override;void SetVolume(double)override;Aemeath::Core::Music::PlaybackStatus Status()const override;private:Aemeath::Core::Music::PlaybackStatus m_status{Aemeath::Core::Music::PlaybackStatus::Stopped};double m_volume{1.0};};class NullTextToSpeech final:public Aemeath::Services::ITextToSpeechService{public:std::vector<Aemeath::Core::Voice::VoiceProfile> ListVoices()override;std::future<void> SpeakAsync(std::string,Aemeath::Core::Voice::VoiceProfile)override;void Stop()override;};}
