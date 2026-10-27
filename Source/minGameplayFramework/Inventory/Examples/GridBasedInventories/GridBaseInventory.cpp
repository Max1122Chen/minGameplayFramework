#include "Inventory/Examples/GridBasedInventories/GridBaseInventory.hpp"

#include <algorithm>
#include <utility>

namespace minGameplayFramework::Inventory::GridBased
{
    void GridGeometryFragment::GrantProperty(InventoryItem::ItemInstance& itemInstance) const
    {
        InventoryItem::IntInstanceProperty property(kRotationPropertyName, minEngine::GameplayTag{});
        property.SetRange(0, 3);
        property.SetValue(0);
        itemInstance.AddProperty(std::move(property));
    }

    int GridGeometryFragment::GetRotationQuarterTurns(const InventoryItem::ItemInstance& item)
    {
        const InventoryItem::InstancePropertyBase* property = item.GetProperty(kRotationPropertyName);
        if (property == nullptr || property->GetPropertyType() != InventoryItem::InstancePropertyType::Int)
        {
            return 0;
        }
        return property->AsIntProperty()->GetValue() & 3;
    }

    void GridGeometryFragment::SetRotationQuarterTurns(InventoryItem::ItemInstance& item, int quarterTurns)
    {
        InventoryItem::InstancePropertyBase* property = item.GetProperty(kRotationPropertyName);
        if (property == nullptr || property->GetPropertyType() != InventoryItem::InstancePropertyType::Int)
        {
            InventoryItem::IntInstanceProperty created(kRotationPropertyName, minEngine::GameplayTag{});
            created.SetRange(0, 3);
            created.SetValue(quarterTurns & 3);
            item.AddProperty(std::move(created));
            return;
        }
        property->AsIntProperty()->SetValue(quarterTurns & 3);
    }

    int GridGeometryFragment::EffectiveWidth(const InventoryItem::ItemInstance& item)
    {
        if (item.GetItemDef() == nullptr)
        {
            return 1;
        }
        const auto* geometry = item.GetItemDef()->GetFragment<GridGeometryFragment>();
        if (geometry == nullptr)
        {
            return 1;
        }
        const int turns = GetRotationQuarterTurns(item);
        return (turns % 2 == 0) ? geometry->GetGridWidth() : geometry->GetGridHeight();
    }

    int GridGeometryFragment::EffectiveHeight(const InventoryItem::ItemInstance& item)
    {
        if (item.GetItemDef() == nullptr)
        {
            return 1;
        }
        const auto* geometry = item.GetItemDef()->GetFragment<GridGeometryFragment>();
        if (geometry == nullptr)
        {
            return 1;
        }
        const int turns = GetRotationQuarterTurns(item);
        return (turns % 2 == 0) ? geometry->GetGridHeight() : geometry->GetGridWidth();
    }

    bool GridBasedInventory::FitsInGrid(const GridPlacement& placement) const
    {
        const int w = GridGeometryFragment::EffectiveWidth(placement.Item);
        const int h = GridGeometryFragment::EffectiveHeight(placement.Item);
        return placement.OriginX >= 0 && placement.OriginY >= 0 &&
               placement.OriginX + w <= m_GridWidth && placement.OriginY + h <= m_GridHeight;
    }

    void GridBasedInventory::SetGridSize(int width, int height)
    {
        const int newW = width > 0 ? width : 0;
        const int newH = height > 0 ? height : 0;
        const int oldW = m_GridWidth;
        const int oldH = m_GridHeight;
        if (newW == oldW && newH == oldH)
        {
            return;
        }

        std::vector<GridPlacement> previous = std::move(m_Placements);
        m_Placements.clear();
        m_GridWidth = newW;
        m_GridHeight = newH;

        InventoryEvictionBatch eviction;
        eviction.Reason = InventoryChangeReason::CapacityShrink;

        for (GridPlacement& placement : previous)
        {
            if (placement.Item.IsEmpty())
            {
                continue;
            }

            if (FitsInGrid(placement) && CanPlace(placement.Item, placement.OriginX, placement.OriginY))
            {
                m_Placements.push_back(std::move(placement));
                continue;
            }

            int x = 0;
            int y = 0;
            int turns = GridGeometryFragment::GetRotationQuarterTurns(placement.Item);
            if (FindFirstFit(placement.Item, x, y, turns))
            {
                GridGeometryFragment::SetRotationQuarterTurns(placement.Item, turns);
                placement.OriginX = x;
                placement.OriginY = y;
                m_Placements.push_back(std::move(placement));
            }
            else
            {
                eviction.Items.push_back(std::move(placement.Item));
            }
        }

        if (!eviction.Items.empty())
        {
            OnItemsEvicted.Broadcast(eviction);
        }
        OnGridSizeChanged.Broadcast(oldW, oldH, newW, newH);
        OnContentsChanged.Broadcast();
    }

    void GridBasedInventory::RebuildOccupancy(std::vector<uint8_t>& occupancy) const
    {
        occupancy.assign(static_cast<size_t>(m_GridWidth * m_GridHeight), 0);
        for (const GridPlacement& placement : m_Placements)
        {
            const int w = GridGeometryFragment::EffectiveWidth(placement.Item);
            const int h = GridGeometryFragment::EffectiveHeight(placement.Item);
            for (int y = 0; y < h; ++y)
            {
                for (int x = 0; x < w; ++x)
                {
                    const int cx = placement.OriginX + x;
                    const int cy = placement.OriginY + y;
                    if (cx < 0 || cy < 0 || cx >= m_GridWidth || cy >= m_GridHeight)
                    {
                        continue;
                    }
                    occupancy[static_cast<size_t>(cy * m_GridWidth + cx)] = 1;
                }
            }
        }
    }

    bool GridBasedInventory::CanPlace(const InventoryItem::ItemInstance& item, int originX, int originY) const
    {
        const int w = GridGeometryFragment::EffectiveWidth(item);
        const int h = GridGeometryFragment::EffectiveHeight(item);
        if (originX < 0 || originY < 0 || originX + w > m_GridWidth || originY + h > m_GridHeight)
        {
            return false;
        }

        std::vector<uint8_t> occupancy;
        RebuildOccupancy(occupancy);
        for (int y = 0; y < h; ++y)
        {
            for (int x = 0; x < w; ++x)
            {
                if (occupancy[static_cast<size_t>((originY + y) * m_GridWidth + (originX + x))] != 0)
                {
                    return false;
                }
            }
        }
        return true;
    }

    bool GridBasedInventory::TryPlaceAt(InventoryItem::ItemInstance item, int originX, int originY, int quarterTurns)
    {
        if (item.IsEmpty())
        {
            return false;
        }
        GridGeometryFragment::SetRotationQuarterTurns(item, quarterTurns);
        if (!CanPlace(item, originX, originY))
        {
            return false;
        }
        GridPlacement placement;
        placement.Item = std::move(item);
        placement.OriginX = originX;
        placement.OriginY = originY;
        m_Placements.push_back(std::move(placement));
        OnContentsChanged.Broadcast();
        return true;
    }

    bool GridBasedInventory::FindFirstFit(const InventoryItem::ItemInstance& item, int& outX, int& outY,
                                          int& outQuarterTurns) const
    {
        InventoryItem::ItemInstance probe = item;
        for (int turns : {0, 1})
        {
            GridGeometryFragment::SetRotationQuarterTurns(probe, turns);
            const int w = GridGeometryFragment::EffectiveWidth(probe);
            const int h = GridGeometryFragment::EffectiveHeight(probe);
            for (int y = 0; y <= m_GridHeight - h; ++y)
            {
                for (int x = 0; x <= m_GridWidth - w; ++x)
                {
                    if (CanPlace(probe, x, y))
                    {
                        outX = x;
                        outY = y;
                        outQuarterTurns = turns;
                        return true;
                    }
                }
            }
        }
        return false;
    }

    int GridBasedInventory::TryAddItem(InventoryItem::ItemInstance& item)
    {
        if (item.IsEmpty())
        {
            return 0;
        }

        // 网格示例默认一次放一整堆为一个占位物（不在格内再拆堆）；接受则清空入参 stack。
        int x = 0;
        int y = 0;
        int turns = 0;
        if (!FindFirstFit(item, x, y, turns))
        {
            return 0;
        }

        const int accepted = item.GetStackCount();
        InventoryItem::ItemInstance placed = item;
        if (!TryPlaceAt(std::move(placed), x, y, turns))
        {
            return 0;
        }
        item.SetStackCount(0);
        item.SetItemDef(nullptr);
        return accepted;
    }

    int GridBasedInventory::TryRemoveItem(InventoryItem::ItemInstance& item)
    {
        if (item.GetItemDef() == nullptr || item.GetStackCount() <= 0)
        {
            return 0;
        }

        int remaining = item.GetStackCount();
        int removed = 0;
        for (auto it = m_Placements.begin(); it != m_Placements.end() && remaining > 0;)
        {
            if (it->Item.GetItemDef() != item.GetItemDef())
            {
                ++it;
                continue;
            }
            const int take = it->Item.GetStackCount() < remaining ? it->Item.GetStackCount() : remaining;
            it->Item.SetStackCount(it->Item.GetStackCount() - take);
            remaining -= take;
            removed += take;
            if (it->Item.GetStackCount() <= 0)
            {
                it = m_Placements.erase(it);
            }
            else
            {
                ++it;
            }
        }
        item.SetStackCount(remaining);
        if (remaining == 0)
        {
            item.SetItemDef(nullptr);
        }
        if (removed > 0)
        {
            OnContentsChanged.Broadcast();
        }
        return removed;
    }

    void GridBasedInventory::TryAutoOrganize()
    {
        std::vector<InventoryItem::ItemInstance> stacks;
        for (GridPlacement& placement : m_Placements)
        {
            stacks.push_back(std::move(placement.Item));
        }
        m_Placements.clear();

        // Merge stackable same-def piles first.
        for (size_t i = 0; i < stacks.size(); ++i)
        {
            if (stacks[i].IsEmpty())
            {
                continue;
            }
            for (size_t j = i + 1; j < stacks.size(); ++j)
            {
                if (!stacks[i].CanStackWith(stacks[j]))
                {
                    continue;
                }
                const int maxStack = stacks[i].GetItemDef()->GetMaxStackCount();
                const int space = maxStack - stacks[i].GetStackCount();
                if (space <= 0)
                {
                    continue;
                }
                const int move = stacks[j].GetStackCount() < space ? stacks[j].GetStackCount() : space;
                stacks[i].SetStackCount(stacks[i].GetStackCount() + move);
                stacks[j].SetStackCount(stacks[j].GetStackCount() - move);
            }
        }

        stacks.erase(std::remove_if(stacks.begin(), stacks.end(),
                                    [](const InventoryItem::ItemInstance& stack) { return stack.IsEmpty(); }),
                     stacks.end());

        std::stable_sort(stacks.begin(), stacks.end(),
                         [](const InventoryItem::ItemInstance& a, const InventoryItem::ItemInstance& b) {
                             const int aw = GridGeometryFragment::EffectiveWidth(a) *
                                            GridGeometryFragment::EffectiveHeight(a);
                             const int bw = GridGeometryFragment::EffectiveWidth(b) *
                                            GridGeometryFragment::EffectiveHeight(b);
                             return aw > bw;
                         });

        InventoryEvictionBatch eviction;
        eviction.Reason = InventoryChangeReason::Reorganized;

        for (InventoryItem::ItemInstance& stack : stacks)
        {
            int x = 0;
            int y = 0;
            int turns = 0;
            if (FindFirstFit(stack, x, y, turns))
            {
                // TryPlaceAt 会 Broadcast；整理末尾再统一通知一次即可。
                GridGeometryFragment::SetRotationQuarterTurns(stack, turns);
                GridPlacement placement;
                placement.Item = std::move(stack);
                placement.OriginX = x;
                placement.OriginY = y;
                m_Placements.push_back(std::move(placement));
            }
            else if (!stack.IsEmpty())
            {
                eviction.Items.push_back(std::move(stack));
            }
        }

        if (!eviction.Items.empty())
        {
            OnItemsEvicted.Broadcast(eviction);
        }
        OnContentsChanged.Broadcast();
    }
}
