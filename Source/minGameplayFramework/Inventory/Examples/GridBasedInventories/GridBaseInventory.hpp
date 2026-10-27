#pragma once

#include "MinGameplayFrameworkAPI.hpp"
#include "Inventory/InventoryBase.hpp"
#include "Inventory/InventoryChangeTypes.hpp"
#include "GameplayContent/InventoryItem/ItemDef.hpp"
#include "GameplayContent/InventoryItem/ItemInstance.hpp"

#include "Runtime/Core/Delegates/MulticastDelegate.h"

#include <cstdint>
#include <string>
#include <vector>

namespace minGameplayFramework::Inventory::GridBased
{
    /// @brief 物品占格尺寸（挂在 ItemDef 上）；当前朝向通过 GrantProperty 写入 Instance。
    /// @note 片段放在本示例目录旁，便于对照阅读 Grid 上下文。
    ME_CLASS()
    class GridGeometryFragment : public minGameplayFramework::InventoryItem::ItemDefFragmentBase
    {
        ME_GENERATED_BODY()

    public:
        static constexpr const char* kRotationPropertyName = "GridRotation";

        std::string GetFragmentId() const override { return "GridGeometry"; }
        minEngine::GameplayTag GetFragmentIdTag() const override { return m_FragmentIdTag; }

        int GetGridWidth() const { return m_GridWidth; }
        int GetGridHeight() const { return m_GridHeight; }
        void SetGridSize(int width, int height)
        {
            m_GridWidth = width > 0 ? width : 1;
            m_GridHeight = height > 0 ? height : 1;
        }

        /// @brief 授予 Instance 旋转属性（0..3，单位 90°）。占格朝向真源在 Instance，不在片段。
        void GrantProperty(InventoryItem::ItemInstance& itemInstance) const;

        static int GetRotationQuarterTurns(const InventoryItem::ItemInstance& item);
        static void SetRotationQuarterTurns(InventoryItem::ItemInstance& item, int quarterTurns);

        static int EffectiveWidth(const InventoryItem::ItemInstance& item);
        static int EffectiveHeight(const InventoryItem::ItemInstance& item);

    private:
        minEngine::GameplayTag m_FragmentIdTag;
        ME_PROPERTY()
        int m_GridWidth = 1;
        ME_PROPERTY()
        int m_GridHeight = 1;
    };

    struct GridPlacement
    {
        InventoryItem::ItemInstance Item;
        int OriginX = 0;
        int OriginY = 0;
    };

    /// @brief 《生化危机4》《逃离塔科夫》式网格背包：占格 + 90° 旋转 + 简单整理。
    /// @note TryAdd 一次放一整堆为一个占位物（不在格内再拆堆）。
    class GridBasedInventory : public IInventoryBase
    {
    public:
        /// @brief 改网格尺寸。缩小：越界或无法重放的物品进入 OnItemsEvicted。
        void SetGridSize(int width, int height);
        int GetGridWidth() const { return m_GridWidth; }
        int GetGridHeight() const { return m_GridHeight; }

        int TryAddItem(InventoryItem::ItemInstance& item) override;
        int TryRemoveItem(InventoryItem::ItemInstance& item) override;

        bool TryPlaceAt(InventoryItem::ItemInstance item, int originX, int originY, int quarterTurns);
        bool FindFirstFit(const InventoryItem::ItemInstance& item, int& outX, int& outY, int& outQuarterTurns) const;

        void TryAutoOrganize();

        const std::vector<GridPlacement>& GetPlacements() const { return m_Placements; }

        minEngine::MulticastDelegate<> OnContentsChanged;
        minEngine::MulticastDelegate<int, int, int, int> OnGridSizeChanged; // oldW,oldH,newW,newH
        minEngine::MulticastDelegate<const InventoryEvictionBatch&> OnItemsEvicted;

    private:
        int m_GridWidth = 0;
        int m_GridHeight = 0;
        std::vector<GridPlacement> m_Placements;

        bool CanPlace(const InventoryItem::ItemInstance& item, int originX, int originY) const;
        void RebuildOccupancy(std::vector<uint8_t>& occupancy) const;
        bool FitsInGrid(const GridPlacement& placement) const;
    };
}

#include "GridBaseInventory.gen.h"
