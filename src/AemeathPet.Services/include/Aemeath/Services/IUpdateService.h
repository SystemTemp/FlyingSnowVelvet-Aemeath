#pragma once
#include "Aemeath/Core/Update/UpdateModels.h"
#include <future>
namespace Aemeath::Services {class IUpdateService{public:virtual~IUpdateService()=default;virtual std::future<Aemeath::Core::Update::UpdateManifest> CheckAsync()=0;virtual std::future<void> DownloadAsync(Aemeath::Core::Update::UpdateManifest manifest,std::string destination)=0;};}
