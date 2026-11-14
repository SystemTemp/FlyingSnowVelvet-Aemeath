#pragma once
#include "Aemeath/Services/IChatService.h"
#include "Aemeath/Services/IHttpClient.h"
#include <memory>
namespace Aemeath::Infrastructure::Chat {
class OpenAICompatibleChatService final:public Aemeath::Services::IChatService{public:OpenAICompatibleChatService(std::shared_ptr<Aemeath::Services::IHttpClient> http,std::string endpoint,std::string apiKey,std::string model);std::future<Aemeath::Core::Chat::ChatMessage> SendAsync(std::vector<Aemeath::Core::Chat::ChatMessage> history,Aemeath::Core::Chat::ChatMessage userMessage)override;private:std::shared_ptr<Aemeath::Services::IHttpClient> m_http;std::string m_endpoint,m_apiKey,m_model;};
}
