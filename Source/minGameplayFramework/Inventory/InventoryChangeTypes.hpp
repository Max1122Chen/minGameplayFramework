#pragma once

#include "MinGameplayFrameworkAPI.hpp"
#include "GameplayContent/InventoryItem/ItemInstance.hpp"

#include <vector>

namespace minGameplayFramework::Inventory
{
    /// @brief 背包内容变化原因，供 UI / 音效区分。
    enum class InventoryChangeReason
    {
        Added,
        Removed,
        Merged,
        Cleared,
        Set,
        Compacted,
        CapacityShrink,
        CapacityExpand,
        Reorganized,
    };

    /// @brief 缩容或溢出时被挤出的物品批次。
    struct InventoryEvictionBatch
    {
        std::vector<InventoryItem::ItemInstance> Items;
        InventoryChangeReason Reason = InventoryChangeReason::CapacityShrink;
    };
}
