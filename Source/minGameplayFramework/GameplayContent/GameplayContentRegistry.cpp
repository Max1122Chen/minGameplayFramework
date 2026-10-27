#include "GameplayContent/GameplayContentRegistry.hpp"

namespace minGameplayFramework::GameplayContent
{
    void GameplayContentCatalog::AddContentDef(std::shared_ptr<GameplayContentDef> def)
    {
        if (def == nullptr)
        {
            return;
        }
        m_ContentDefs.push_back(std::move(def));
        RebuildLookupMaps();
    }

    GameplayContentDef* GameplayContentCatalog::GetContentDef(int contentId) const
    {
        const auto it = m_IdToDef.find(contentId);
        return it != m_IdToDef.end() ? it->second : nullptr;
    }

    GameplayContentDef* GameplayContentCatalog::GetContentDef(const std::string& contentName) const
    {
        const auto it = m_NameToDef.find(contentName);
        return it != m_NameToDef.end() ? it->second : nullptr;
    }

    void GameplayContentCatalog::RebuildLookupMaps()
    {
        m_IdToDef.clear();
        m_NameToDef.clear();
        for (const auto& def : m_ContentDefs)
        {
            if (def == nullptr)
            {
                continue;
            }
            m_IdToDef[def->GetContentId()] = def.get();
            if (!def->GetContentName().empty())
            {
                m_NameToDef[def->GetContentName()] = def.get();
            }
        }
    }

    void GameplayContentRegistry::Initialize(minEngine::SubsystemCollectionBase& collection)
    {
        (void)collection;
        RebuildLookupMaps();
    }

    void GameplayContentRegistry::Deinitialize()
    {
        m_Catalogs.clear();
        m_IdToDef.clear();
        m_NameToDef.clear();
    }

    void GameplayContentRegistry::RegisterCatalog(std::shared_ptr<GameplayContentCatalog> catalog)
    {
        if (catalog == nullptr)
        {
            return;
        }
        m_Catalogs.push_back(std::move(catalog));
        RebuildLookupMaps();
    }

    GameplayContentDef* GameplayContentRegistry::FindContentDef(int contentId) const
    {
        const auto it = m_IdToDef.find(contentId);
        return it != m_IdToDef.end() ? it->second : nullptr;
    }

    GameplayContentDef* GameplayContentRegistry::FindContentDef(const std::string& contentName) const
    {
        const auto it = m_NameToDef.find(contentName);
        return it != m_NameToDef.end() ? it->second : nullptr;
    }

    void GameplayContentRegistry::RebuildLookupMaps()
    {
        m_IdToDef.clear();
        m_NameToDef.clear();
        for (const auto& catalog : m_Catalogs)
        {
            if (catalog == nullptr)
            {
                continue;
            }
            for (const auto& def : catalog->GetContentDefs())
            {
                if (def == nullptr)
                {
                    continue;
                }
                m_IdToDef[def->GetContentId()] = def.get();
                if (!def->GetContentName().empty())
                {
                    m_NameToDef[def->GetContentName()] = def.get();
                }
            }
        }
    }
}
