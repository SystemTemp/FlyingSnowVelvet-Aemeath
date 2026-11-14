#include "Aemeath/Application/AppController.h"

#include "Aemeath/Core/Logging/Logger.h"

namespace Aemeath::Application {

AppController::AppController(
    std::shared_ptr<Aemeath::Services::ISettingsStore> store,
    std::shared_ptr<Aemeath::Services::IChatService> chat,
    std::shared_ptr<Aemeath::Services::IForumService> forum)
    : m_store(std::move(store)),
      m_chat(std::move(chat)),
      m_forum(std::move(forum)) {
}

void AppController::Initialize() {
    if (m_store) {
        m_settings = m_store->Load();
        Aemeath::Core::Logging::Logger::Instance().Info("app", "设置已加载");
    }
    if (!m_settings.pet.characterId.empty()) {
        Aemeath::Core::Logging::Logger::Instance().Info("app", "当前宠物：" + m_settings.pet.characterId);
    }
}

void AppController::SaveSettings() {
    if (!m_store) {
        return;
    }
    m_settings.pet.x = m_settings.pet.x;
    m_store->Save(m_settings);
    Aemeath::Core::Logging::Logger::Instance().Info("app", "设置已保存");
}

std::future<Aemeath::Core::Chat::ChatMessage> AppController::SendChatAsync(
    std::vector<Aemeath::Core::Chat::ChatMessage> history,
    Aemeath::Core::Chat::ChatMessage message) {
    if (!m_chat) {
        return std::async(std::launch::async, [] {
            Aemeath::Core::Chat::ChatMessage reply;
            reply.id = "no-service";
            reply.role = Aemeath::Core::Chat::ChatRole::Assistant;
            reply.content = "聊天服务未初始化。";
            return reply;
        });
    }
    return m_chat->SendAsync(std::move(history), std::move(message));
}

std::future<Aemeath::Core::Forum::ForumPostPage> AppController::SearchWallAsync(
    std::string keyword,
    int page) {
    if (!m_forum) {
        return std::async(std::launch::async, [] {
            return Aemeath::Core::Forum::ForumPostPage {};
        });
    }
    Aemeath::Services::ForumPostQuery query;
    query.page = page;
    query.perPage = m_settings.forum.pageSize > 0 ? m_settings.forum.pageSize : 20;
    query.sort = "new";
    if (!keyword.empty()) {
        query.keyword = std::move(keyword);
    }
    return m_forum->ListPostsAsync(std::move(query));
}

} // namespace Aemeath::Application