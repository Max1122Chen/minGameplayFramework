#pragma once

#include "MinGameplayFrameworkAPI.hpp"
#include "GameplayContent/InventoryItem/ItemDef.hpp"
#include "GameplayContent/InventoryItem/ItemInstance.hpp"

#include "Runtime/Function/GameplayFramework/Tags/GameplayTag.h"

#include <string>

namespace minGameplayFramework::InventoryItem
{
    /// @brief 示例片段：演示组合扩展。堆叠请用 ItemDef::m_MaxStackCount，勿再引入 StackableFragment。

    /// @brief 稀有度标记（Tag）。
    ME_CLASS()
    class RarityFragment : public ItemDefFragmentBase
    {
        ME_GENERATED_BODY()

    public:
        std::string GetFragmentId() const override { return "Rarity"; }
        minEngine::GameplayTag GetFragmentIdTag() const override { return m_FragmentIdTag; }

        const minEngine::GameplayTag& GetRarity() const { return m_Rarity; }
        void SetRarity(minEngine::GameplayTag rarity) { m_Rarity = std::move(rarity); }

    private:
        minEngine::GameplayTag m_FragmentIdTag;
        ME_PROPERTY()
        minEngine::GameplayTag m_Rarity;
    };

    /// @brief 带有本片段即表示可被消耗/使用；效果钩子待 GAS 后再补。
    ME_CLASS()
    class ConsumableFragment : public ItemDefFragmentBase
    {
        ME_GENERATED_BODY()

    public:
        std::string GetFragmentId() const override { return "Consumable"; }
        minEngine::GameplayTag GetFragmentIdTag() const override { return m_FragmentIdTag; }

    private:
        minEngine::GameplayTag m_FragmentIdTag;
    };

    /// @brief 带有本片段即表示可放置到场景。
    ME_CLASS()
    class PlaceableFragment : public ItemDefFragmentBase
    {
        ME_GENERATED_BODY()

    public:
        std::string GetFragmentId() const override { return "Placeable"; }
        minEngine::GameplayTag GetFragmentIdTag() const override { return m_FragmentIdTag; }

    private:
        minEngine::GameplayTag m_FragmentIdTag;
    };

    /// @brief 带有本片段即表示可装备到角色。
    ME_CLASS()
    class EquipableFragment : public ItemDefFragmentBase
    {
        ME_GENERATED_BODY()

    public:
        std::string GetFragmentId() const override { return "Equipable"; }
        minEngine::GameplayTag GetFragmentIdTag() const override { return m_FragmentIdTag; }

    private:
        minEngine::GameplayTag m_FragmentIdTag;
    };

    /// @brief 耐久 Schema：只存上限；当前耐久通过 GrantProperty 写入 ItemInstance。
    ME_CLASS()
    class DurabilityFragment : public ItemDefFragmentBase
    {
        ME_GENERATED_BODY()

    public:
        static constexpr const char* kPropertyName = "Durability";

        std::string GetFragmentId() const override { return "Durability"; }
        minEngine::GameplayTag GetFragmentIdTag() const override { return m_FragmentIdTag; }

        int GetMaxDurability() const { return m_MaxDurability; }
        void SetMaxDurability(int maxDurability) { m_MaxDurability = maxDurability; }

        /// @brief 向实例授予当前耐久属性（真源在 Instance，不在片段）。
        void GrantProperty(ItemInstance& itemInstance) const;

    private:
        minEngine::GameplayTag m_FragmentIdTag;
        ME_PROPERTY()
        int m_MaxDurability = 0;
    };
}

#include "ExampleItemDefFragments.gen.h"
