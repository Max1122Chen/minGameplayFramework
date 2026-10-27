#pragma once

#include "MinGameplayFrameworkAPI.hpp"
#include "Inventory/InventoryBase.hpp"
#include "Inventory/InventoryChangeTypes.hpp"
#include "GameplayContent/InventoryItem/ItemDef.hpp"
#include "GameplayContent/InventoryItem/ItemInstance.hpp"

#include "Runtime/Core/Delegates/MulticastDelegate.h"
#include "Runtime/Function/GameplayFramework/Tags/GameplayTag.h"

#include <map>
#include <vector>

namespace minGameplayFramework::Inventory::CategoryBased
{
    /// @brief 单一类别容器（按 Tag **名**过滤）；槽语义与 SlotBased 对齐。
    /// @note 过滤键是 `GameplayTag::GetName()`，不要求 Tag 已注册到 TagManager。
    ///       类别名为空时视为开放桶（不校验）。
    class CategorizedInventory : public IInventoryBase
    {
    public:
        void SetCategory(minEngine::GameplayTag category) { m_Category = std::move(category); }
        const minEngine::GameplayTag& GetCategory() const { return m_Category; }

        void SetCapacity(int capacity);
        int GetCapacity() const { return m_Capacity; }

        const InventoryItem::ItemInstance& GetSlot(int index) const;
        /// @brief 写入槽位；非空且类别 Tag 名与本容器不符时拒绝，槽位不变。
        /// @return 是否写入成功。
        bool SetSlot(int index, InventoryItem::ItemInstance item);
        void ClearSlot(int index);

        int TryAddItem(InventoryItem::ItemInstance& item) override;
        int TryRemoveItem(InventoryItem::ItemInstance& item) override;
        int TryAddTo(int index, InventoryItem::ItemInstance& item);
        InventoryItem::ItemInstance TryRemoveFrom(int index, int count);

        minEngine::MulticastDelegate<int, InventoryChangeReason> OnSlotChanged;
        minEngine::MulticastDelegate<int, int> OnCapacityChanged;
        minEngine::MulticastDelegate<const InventoryEvictionBatch&> OnItemsEvicted;

    private:
        int TryAddIntoRange(InventoryItem::ItemInstance& item, int begin, int end, bool broadcast);
        void EmitSlotChanged(int index, InventoryChangeReason reason);

        int m_Capacity = 0;
        minEngine::GameplayTag m_Category;
        std::vector<InventoryItem::ItemInstance> m_Items;
    };

    /// @brief 《上古卷轴5》《辐射4》《旷野之息》式分类背包：按 Item 类别 Tag 路由到子容器。
    class CategoryBasedInventory : public IInventoryBase
    {
    public:
        CategorizedInventory& EnsureCategory(const minEngine::GameplayTag& category, int capacity);

        int TryAddItem(InventoryItem::ItemInstance& item) override;
        int TryRemoveItem(InventoryItem::ItemInstance& item) override;

    private:
        std::map<std::string, CategorizedInventory> m_CategoryInventories;
    };
}
