#include "Inventory/Examples/SlotBasedInventories/SlotBasedInventory.hpp"

#include <utility>

namespace minGameplayFramework::Inventory::SlotBased
{
    void SlotBasedInventory::EmitSlotChanged(int index, InventoryChangeReason reason)
    {
        OnSlotChanged.Broadcast(index, reason);
    }

    void SlotBasedInventory::SetCapacity(int capacity)
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

        // Shrink: relocate items in [newCapacity, oldCapacity) into [0, newCapacity).
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

            // Temporarily keep full vector so TryAddIntoRange can write front slots.
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

    const InventoryItem::ItemInstance& SlotBasedInventory::GetSlot(int index) const
    {
        return m_Items.at(static_cast<size_t>(index));
    }

    void SlotBasedInventory::SetSlot(int index, InventoryItem::ItemInstance item)
    {
        m_Items.at(static_cast<size_t>(index)) = std::move(item);
        EmitSlotChanged(index, InventoryChangeReason::Set);
    }

    void SlotBasedInventory::ClearSlot(int index)
    {
        InventoryItem::ItemInstance& slot = m_Items.at(static_cast<size_t>(index));
        if (slot.IsEmpty())
        {
            return;
        }
        slot = InventoryItem::ItemInstance{};
        EmitSlotChanged(index, InventoryChangeReason::Cleared);
    }

    int SlotBasedInventory::TryAddItem(InventoryItem::ItemInstance& item)
    {
        return TryAddIntoRange(item, 0, GetStorageEnd(), true);
    }

    int SlotBasedInventory::TryAddTo(int index, InventoryItem::ItemInstance& item)
    {
        if (item.IsEmpty() || item.GetItemDef() == nullptr)
        {
            return 0;
        }
        if (index < 0 || index >= m_Capacity)
        {
            return 0;
        }

        const int maxStack = item.GetItemDef()->GetMaxStackCount();
        InventoryItem::ItemInstance& slot = m_Items[static_cast<size_t>(index)];
        int accepted = 0;

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
            accepted = move;
            EmitSlotChanged(index, InventoryChangeReason::Added);
            return accepted;
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
        accepted = move;
        EmitSlotChanged(index, InventoryChangeReason::Merged);
        return accepted;
    }

    InventoryItem::ItemInstance SlotBasedInventory::TryRemoveFrom(int index, int count)
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

    int SlotBasedInventory::TryAddIntoRange(InventoryItem::ItemInstance& item, int begin, int end, bool broadcast)
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
            // 深拷贝属性袋：新占位堆继承入参实例的 Instance Property。
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

    int SlotBasedInventory::TryRemoveItem(InventoryItem::ItemInstance& item)
    {
        if (item.GetItemDef() == nullptr || item.GetStackCount() <= 0)
        {
            return 0;
        }

        int remaining = item.GetStackCount();
        int removed = 0;
        const int end = GetStorageEnd();

        for (int i = 0; i < end && remaining > 0; ++i)
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

    MinecraftPlayerInventory::MinecraftPlayerInventory()
    {
        SetCapacity(41);
    }

    Chest::Chest()
    {
        SetCapacity(27);
    }
}
