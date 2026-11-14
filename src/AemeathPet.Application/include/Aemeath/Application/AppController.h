#pragma once
#include "Aemeath/Services/IChatService.h"
#include "Aemeath/Services/IForumService.h"
#include "Aemeath/Services/ISettingsStore.h"
#include <memory>
namespace Aemeath::Application {class AppController{public:AppController(std::shared_ptr<Aemeath::Services::ISettingsStore>,std::shared_ptr<Aemeath::Services::IChatService>,std::shared_ptr<Aemeath::Services::IForumService>);void Initialize();void SaveSettings();Aemeath::Core::Config::AppSettings& Settings()noexcept{return m_settings;}std::future<Aemeath::Core::Chat::ChatMessage> SendChatAsync(std::vector<Aemeath::Core::Chat::ChatMessage> h,Aemeath::Core::Chat::ChatMessage m);std::future<Aemeath::Core::Forum::ForumPostPage> SearchWallAsync(std::string keyword,int page=1);private:std::shared_ptr<Aemeath::Services::ISettingsStore>m_store;std::shared_ptr<Aemeath::Services::IChatService>m_chat;std::shared_ptr<Aemeath::Services::IForumService>m_forum;Aemeath::Core::Config::AppSettings m_settings;};}
