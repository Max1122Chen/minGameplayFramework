#include "Inventory/Examples/CategoryBasedInventories/CategoryBasedInventory.hpp"

#include <utility>

namespace minGameplayFramework::Inventory::CategoryBased
{
    void CategorizedInventory::EmitSlotChanged(int index, InventoryChangeReason reason)
    {
        OnSlotChanged.Broadcast(index, reason);
    }

    void CategorizedInventory::SetCapacity(int capacity)
    {
        const int newCapacity = capacity < 0 ? 0 : capacity;
        const int oldCapacity = m_Capacity;
        if (newCapacity == oldCapacity)
        {
            return;
        }

        if (newCapacity > oldCapacity)
        {
            m_Capacity = newCapacity;
            m_Items.resize(static_cast<size_t>(m_Capacity));
            OnCapacityChanged.Broadcast(oldCapacity, newCapacity);
            return;
        }

        InventoryEvictionBatch eviction;
        eviction.Reason = InventoryChangeReason::CapacityShrink;

        for (int i = newCapacity; i < oldCapacity; ++i)
        {
            InventoryItem::ItemInstance& trailing = m_Items[static_cast<size_t>(i)];
            if (trailing.IsEmpty())
            {
                continue;
            }

            InventoryItem::ItemInstance moving = std::move(trailing);
            trailing = InventoryItem::ItemInstance{};
            TryAddIntoRange(moving, 0, newCapacity, true);
            if (!moving.IsEmpty())
            {
                eviction.Items.push_back(std::move(moving));
            }
        }

        m_Capacity = newCapacity;
        m_Items.resize(static_cast<size_t>(m_Capacity));

        if (!eviction.Items.empty())
        {
            OnItemsEvicted.Broadcast(eviction);
        }
        OnCapacityChanged.Broadcast(oldCapacity, newCapacity);
    }

    const InventoryItem::ItemInstance& CategorizedInventory::GetSlot(int index) const
    {
        return m_Items.at(static_cast<size_t>(index));
    }

    void CategorizedInventory::SetSlot(int index, InventoryItem::ItemInstance item)
    {
        m_Items.at(static_cast<size_t>(index)) = std::move(item);
        EmitSlotChanged(index, InventoryChangeReason::Set);
    }

    void CategorizedInventory::ClearSlot(int index)
    {
        InventoryItem::ItemInstance& slot = m_Items.at(static_cast<size_t>(index));
        if (slot.IsEmpty())
        {
            return;
        }
        slot = InventoryItem::ItemInstance{};
        EmitSlotChanged(index, InventoryChangeReason::Cleared);
    }

    int CategorizedInventory::TryAddItem(InventoryItem::ItemInstance& item)
    {
        if (item.IsEmpty() || item.GetItemDef() == nullptr)
        {
            return 0;
        }
        if (m_Category.IsValid() && item.GetItemDef()->GetItemCategoryTag() != m_Category)
        {
            return 0;
        }
        return TryAddIntoRange(item, 0, m_Capacity, true);
    }

    int CategorizedInventory::TryAddTo(int index, InventoryItem::ItemInstance& item)
    {
        if (item.IsEmpty() || item.GetItemDef() == nullptr)
        {
            return 0;
        }
        if (m_Category.IsValid() && item.GetItemDef()->GetItemCategoryTag() != m_Category)
        {
            return 0;
        }
        if (index < 0 || index >= m_Capacity)
        {
            return 0;
        }

        const int maxStack = item.GetItemDef()->GetMaxStackCount();
        InventoryItem::ItemInstance& slot = m_Items[static_cast<size_t>(index)];

        if (slot.IsEmpty())
        {
            const int move = item.GetStackCount() < maxStack ? item.GetStackCount() : maxStack;
            InventoryItem::ItemInstance placed = item;
            placed.SetStackCount(move);
            item.SetStackCount(item.GetStackCount() - move);
            if (item.GetStackCount() == 0)
            {
                item.SetItemDef(nullptr);
            }
            slot = std::move(placed);
            EmitSlotChanged(index, InventoryChangeReason::Added);
            return move;
        }

        if (!slot.CanStackWith(item))
        {
            return 0;
        }

        const int space = maxStack - slot.GetStackCount();
        if (space <= 0)
        {
            return 0;
        }
        const int move = item.GetStackCount() < space ? item.GetStackCount() : space;
        slot.SetStackCount(slot.GetStackCount() + move);
        item.SetStackCount(item.GetStackCount() - move);
        if (item.GetStackCount() == 0)
        {
            item.SetItemDef(nullptr);
        }
        EmitSlotChanged(index, InventoryChangeReason::Merged);
        return move;
    }

    InventoryItem::ItemInstance CategorizedInventory::TryRemoveFrom(int index, int count)
    {
        InventoryItem::ItemInstance taken;
        if (count <= 0 || index < 0 || index >= m_Capacity)
        {
            return taken;
        }
        InventoryItem::ItemInstance& slot = m_Items[static_cast<size_t>(index)];
        if (slot.IsEmpty())
        {
            return taken;
        }
        const int move = slot.GetStackCount() < count ? slot.GetStackCount() : count;
        taken = slot;
        taken.SetStackCount(move);
        slot.SetStackCount(slot.GetStackCount() - move);
        if (slot.GetStackCount() == 0)
        {
            slot = InventoryItem::ItemInstance{};
            EmitSlotChanged(index, InventoryChangeReason::Cleared);
        }
        else
        {
            EmitSlotChanged(index, InventoryChangeReason::Removed);
        }
        return taken;
    }

    int CategorizedInventory::TryAddIntoRange(InventoryItem::ItemInstance& item, int begin, int end, bool broadcast)
    {
        if (item.IsEmpty() || item.GetItemDef() == nullptr)
        {
            return 0;
        }
        if (begin < 0)
        {
            begin = 0;
        }
        if (end > m_Capacity)
        {
            end = m_Capacity;
        }
        if (begin >= end)
        {
            return 0;
        }

        const int maxStack = item.GetItemDef()->GetMaxStackCount();
        int accepted = 0;

        for (int i = begin; i < end && item.GetStackCount() > 0; ++i)
        {
            InventoryItem::ItemInstance& slot = m_Items[static_cast<size_t>(i)];
            if (!slot.CanStackWith(item))
            {
                continue;
            }
            const int space = maxStack - slot.GetStackCount();
            if (space <= 0)
            {
                continue;
            }
            const int move = item.GetStackCount() < space ? item.GetStackCount() : space;
            slot.SetStackCount(slot.GetStackCount() + move);
            item.SetStackCount(item.GetStackCount() - move);
            accepted += move;
            if (broadcast)
            {
                EmitSlotChanged(i, InventoryChangeReason::Merged);
            }
        }

        for (int i = begin; i < end && item.GetStackCount() > 0; ++i)
        {
            InventoryItem::ItemInstance& slot = m_Items[static_cast<size_t>(i)];
            if (!slot.IsEmpty())
            {
                continue;
            }
            const int move = item.GetStackCount() < maxStack ? item.GetStackCount() : maxStack;
            InventoryItem::ItemInstance placed = item;
            placed.SetStackCount(move);
            item.SetStackCount(item.GetStackCount() - move);
            slot = std::move(placed);
            accepted += move;
            if (broadcast)
            {
                EmitSlotChanged(i, InventoryChangeReason::Added);
            }
        }

        if (item.GetStackCount() == 0)
        {
            item.SetItemDef(nullptr);
        }
        return accepted;
    }

    int CategorizedInventory::TryRemoveItem(InventoryItem::ItemInstance& item)
    {
        if (item.GetItemDef() == nullptr || item.GetStackCount() <= 0)
        {
            return 0;
        }

        int remaining = item.GetStackCount();
        int removed = 0;
        for (int i = 0; i < m_Capacity && remaining > 0; ++i)
        {
            InventoryItem::ItemInstance& slot = m_Items[static_cast<size_t>(i)];
            if (slot.GetItemDef() != item.GetItemDef() || slot.IsEmpty())
            {
                continue;
            }
            const int take = slot.GetStackCount() < remaining ? slot.GetStackCount() : remaining;
            slot.SetStackCount(slot.GetStackCount() - take);
            if (slot.GetStackCount() == 0)
            {
                slot = InventoryItem::ItemInstance{};
                EmitSlotChanged(i, InventoryChangeReason::Cleared);
            }
            else
            {
                EmitSlotChanged(i, InventoryChangeReason::Removed);
            }
            remaining -= take;
            removed += take;
        }
        item.SetStackCount(remaining);
        if (remaining == 0)
        {
            item.SetItemDef(nullptr);
        }
        return removed;
    }

    CategorizedInventory& CategoryBasedInventory::EnsureCategory(const minEngine::GameplayTag& category,
                                                                int capacity)
    {
        CategorizedInventory& bucket = m_CategoryInventories[category.GetName()];
        bucket.SetCategory(category);
        if (bucket.GetCapacity() != capacity)
        {
            bucket.SetCapacity(capacity);
        }
        return bucket;
    }

    int CategoryBasedInventory::TryAddItem(InventoryItem::ItemInstance& item)
    {
        if (item.IsEmpty() || item.GetItemDef() == nullptr)
        {
            return 0;
        }

        const minEngine::GameplayTag& category = item.GetItemDef()->GetItemCategoryTag();
        if (!category.IsValid())
        {
            return 0;
        }

        auto it = m_CategoryInventories.find(category.GetName());
        if (it == m_CategoryInventories.end())
        {
            return 0;
        }
        return it->second.TryAddItem(item);
    }

    int CategoryBasedInventory::TryRemoveItem(InventoryItem::ItemInstance& item)
    {
        if (item.GetItemDef() == nullptr)
        {
            return 0;
        }
        const minEngine::GameplayTag& category = item.GetItemDef()->GetItemCategoryTag();
        auto it = m_CategoryInventories.find(category.GetName());
        if (it == m_CategoryInventories.end())
        {
            return 0;
        }
        return it->second.TryRemoveItem(item);
    }
}
