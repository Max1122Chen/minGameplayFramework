#pragma once

#include "MinGameplayFrameworkAPI.hpp"
#include "Runtime/Function/Framework/GameInstance/GameInstance.h"

namespace minGameplayFramework
{
    /// @brief Session hub. Set ProjectSettings.GameInstanceClass to
    /// "minGameplayFramework::MinGameInstance".
    ME_CLASS()
    class MinGameInstance : public minEngine::GameInstance
    {
        ME_GENERATED_BODY()

    public:
        void OnInit() override;
        void OnShutdown() override;
    };
}

#include "MinGameInstance.gen.h"
