#pragma once

#include "Aemeath/Services/IChatProvider.h"
#include "Aemeath/Services/IStreamingChatService.h"

#include "Aemeath/Core/Config/AppSettings.h"

namespace Aemeath::Infrastructure::Chat {

/// 把 AppSettings::ChatSettings 映射成 ChatProviderConfig 并构造服务。
std::shared_ptr<Aemeath::Services::IChatService> CreateChatService(
    const Aemeath::Core::Config::ChatSettings& settings,
    std::shared_ptr<Aemeath::Services::IHttpClient> httpClient);

} // namespace Aemeath::Infrastructure::Chat