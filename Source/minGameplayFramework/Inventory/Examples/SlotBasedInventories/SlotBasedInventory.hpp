#pragma once

#include "MinGameplayFrameworkAPI.hpp"
#include "Inventory/InventoryBase.hpp"
#include "Inventory/InventoryChangeTypes.hpp"
#include "GameplayContent/InventoryItem/ItemDef.hpp"
#include "GameplayContent/InventoryItem/ItemInstance.hpp"

#include "Runtime/Core/Delegates/MulticastDelegate.h"

#include <vector>

namespace minGameplayFramework::Inventory::SlotBased
{
    /// @brief 《我的世界》《泰拉瑞亚》式定长槽背包：下标即 slot；先合并再填空。
    class SlotBasedInventory : public IInventoryBase
    {
    public:
        ~SlotBasedInventory() override = default;

        /// @brief 改容量。缩小：先把尾部物品压入前方空槽/可合并槽，放不下的进 OnItemsEvicted。
        void SetCapacity(int capacity);
        int GetCapacity() const { return m_Capacity; }

        const InventoryItem::ItemInstance& GetSlot(int index) const;
        /// @brief 写入整槽并广播 OnSlotChanged（禁止通过可变引用静默改槽）。
        void SetSlot(int index, InventoryItem::ItemInstance item);
        void ClearSlot(int index);

        int TryAddItem(InventoryItem::ItemInstance& item) override;
        int TryRemoveItem(InventoryItem::ItemInstance& item) override;

        /// @brief 只对单槽尝试放入（合并或占据空槽）。
        int TryAddTo(int index, InventoryItem::ItemInstance& item);
        /// @brief 从单槽移除最多 count 个；返回实际取出的堆（含属性拷贝语义由槽内实例决定）。
        InventoryItem::ItemInstance TryRemoveFrom(int index, int count);

        /// @brief 自动填充右开上界；子类可缩小以避免写入装备等特殊区。
        virtual int GetStorageEnd() const { return m_Capacity; }

        minEngine::MulticastDelegate<int, InventoryChangeReason> OnSlotChanged;
        minEngine::MulticastDelegate<int, int> OnCapacityChanged; // old, new
        minEngine::MulticastDelegate<const InventoryEvictionBatch&> OnItemsEvicted;

    protected:
        int TryAddIntoRange(InventoryItem::ItemInstance& item, int begin, int end, bool broadcast);
        void EmitSlotChanged(int index, InventoryChangeReason reason);

        int m_Capacity = 0;
        std::vector<InventoryItem::ItemInstance> m_Items;
    };

    /// @brief Minecraft 风格玩家包区间示意。
    class MinecraftPlayerInventory : public SlotBasedInventory
    {
    public:
        static constexpr int kHotbarBegin = 0;
        static constexpr int kHotbarEnd = 9;
        static constexpr int kMainBegin = 9;
        static constexpr int kMainEnd = 36;
        static constexpr int kArmorBegin = 36;
        static constexpr int kArmorEnd = 40;
        static constexpr int kOffhand = 40;

        MinecraftPlayerInventory();

        int GetStorageEnd() const override { return kMainEnd; }
    };

    /// @brief 普通箱子（示意）。
    class Chest : public SlotBasedInventory
    {
    public:
        Chest();
    };
}
