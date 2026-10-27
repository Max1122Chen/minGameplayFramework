#pragma once

#include "MinGameplayFrameworkAPI.hpp"
#include "GameplayContent/GameplayContentDef.hpp"

#include <string>

namespace minGameplayFramework::InventoryItem
{
    /// @brief Item 专用片段基类：把片段限制在 ItemDef 上，避免泄漏到武器/装备等其它 ContentDef。
    ME_CLASS(Abstract)
    class ItemDefFragmentBase : public minGameplayFramework::GameplayContent::GameplayContentFragment
    {
        ME_GENERATED_BODY()
    };

    /// @brief 物品模板（Schema）。实例是 ItemInstance。
    /// @note 堆叠上限以 m_MaxStackCount 为唯一真源（极常用，不再另做 StackableFragment，避免双份数据）。
    ME_CLASS()
    class ItemDef : public minGameplayFramework::GameplayContent::GameplayContentDef
    {
        ME_GENERATED_BODY()

    public:
        const std::string& GetItemCategory() const { return m_ItemCategory; }
        void SetItemCategory(std::string category) { m_ItemCategory = std::move(category); }

        const minEngine::GameplayTag& GetItemCategoryTag() const { return m_ItemCategoryTag; }
        void SetItemCategoryTag(minEngine::GameplayTag tag) { m_ItemCategoryTag = std::move(tag); }

        const std::string& GetItemDescription() const { return m_ItemDescription; }
        void SetItemDescription(std::string description) { m_ItemDescription = std::move(description); }

        /// @brief 单格最大堆叠；默认 1（无显式配置时更安全）。
        int GetMaxStackCount() const { return m_MaxStackCount; }
        void SetMaxStackCount(int maxStackCount) { m_MaxStackCount = maxStackCount > 0 ? maxStackCount : 1; }

    private:
        ME_PROPERTY()
        std::string m_ItemCategory;

        ME_PROPERTY()
        minEngine::GameplayTag m_ItemCategoryTag;

        ME_PROPERTY()
        std::string m_ItemDescription;

        ME_PROPERTY()
        int m_MaxStackCount = 1;
    };
}

#include "ItemDef.gen.h"
