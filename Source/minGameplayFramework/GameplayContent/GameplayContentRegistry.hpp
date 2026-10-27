#pragma once

#include "MinGameplayFrameworkAPI.hpp"
#include "GameplayContent/GameplayContentDef.hpp"

#include "Runtime/Function/Framework/Subsystems/Subsystem.h"
#include "Runtime/Resource/DataAsset.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace minGameplayFramework::GameplayContent
{
    /// @brief 一类 ContentDef 的目录（引擎侧可作 DataAsset；本仓示例多用伪数据表达同样关系）。
    ME_CLASS()
    class GameplayContentCatalog : public minEngine::DataAsset
    {
        ME_GENERATED_BODY()

    public:
        const minEngine::GameplayTag& GetCatalogTag() const { return m_CatalogTag; }
        void SetCatalogTag(minEngine::GameplayTag tag) { m_CatalogTag = std::move(tag); }

        const std::string& GetCatalogName() const { return m_CatalogName; }
        void SetCatalogName(std::string name) { m_CatalogName = std::move(name); }

        void AddContentDef(std::shared_ptr<GameplayContentDef> def);
        GameplayContentDef* GetContentDef(int contentId) const;
        GameplayContentDef* GetContentDef(const std::string& contentName) const;

        const std::vector<std::shared_ptr<GameplayContentDef>>& GetContentDefs() const
        {
            return m_ContentDefs;
        }

    private:
        ME_PROPERTY()
        minEngine::GameplayTag m_CatalogTag;

        ME_PROPERTY()
        std::string m_CatalogName;

        std::vector<std::shared_ptr<GameplayContentDef>> m_ContentDefs;
        std::unordered_map<int, GameplayContentDef*> m_IdToDef;
        std::unordered_map<std::string, GameplayContentDef*> m_NameToDef;

        void RebuildLookupMaps();
    };

    /// @brief 进程级内容注册表，挂在 GameInstance 上；聚合多个 Catalog。
    /// @note 解决「全进程如何按 Id/Name 找到 Def」；不负责背包或战斗结算。
    ME_CLASS()
    class GameplayContentRegistry : public minEngine::GameInstanceSubsystem
    {
        ME_GENERATED_BODY()

    public:
        void Initialize(minEngine::SubsystemCollectionBase& collection) override;
        void Deinitialize() override;

        void RegisterCatalog(std::shared_ptr<GameplayContentCatalog> catalog);
        GameplayContentDef* FindContentDef(int contentId) const;
        GameplayContentDef* FindContentDef(const std::string& contentName) const;

    private:
        std::vector<std::shared_ptr<GameplayContentCatalog>> m_Catalogs;
        std::unordered_map<int, GameplayContentDef*> m_IdToDef;
        std::unordered_map<std::string, GameplayContentDef*> m_NameToDef;

        void RebuildLookupMaps();
    };
}

#include "GameplayContentRegistry.gen.h"
