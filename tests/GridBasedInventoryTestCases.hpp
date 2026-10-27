#pragma once

#include "doctest.h"

#include "GameplayContent/InventoryItem/ItemDef.hpp"
#include "GameplayContent/InventoryItem/ItemInstance.hpp"
#include "Inventory/Examples/GridBasedInventories/GridBaseInventory.hpp"
#include "Inventory/InventoryChangeTypes.hpp"

#include <memory>

TEST_CASE("grid inventory shrink re-fits or evicts placements")
{
    using namespace minGameplayFramework::InventoryItem;
    using namespace minGameplayFramework::Inventory;
    using namespace minGameplayFramework::Inventory::GridBased;

    auto geometry = std::make_shared<GridGeometryFragment>();
    geometry->SetGridSize(2, 2);

    ItemDef bigDef;
    bigDef.SetMaxStackCount(1);
    REQUIRE(bigDef.AddFragment(geometry));

    GridBasedInventory inventory;
    inventory.SetGridSize(4, 4);

    ItemInstance item;
    item.SetItemDef(&bigDef);
    item.SetStackCount(1);
    geometry->GrantProperty(item);

    REQUIRE(inventory.TryPlaceAt(item, 2, 2, 0));
    CHECK(inventory.GetPlacements().size() == 1);

    std::vector<ItemInstance> evicted;
    inventory.OnItemsEvicted.AddLambda([&](const InventoryEvictionBatch& batch) {
        evicted = batch.Items;
    });

    // Shrink to 2x2: origin (2,2) with 2x2 footprint no longer fits; FirstFit may place at 0,0.
    inventory.SetGridSize(2, 2);
    if (evicted.empty())
    {
        REQUIRE(inventory.GetPlacements().size() == 1);
        CHECK(inventory.GetPlacements()[0].OriginX == 0);
        CHECK(inventory.GetPlacements()[0].OriginY == 0);
    }
    else
    {
        CHECK(evicted.size() == 1);
        CHECK(inventory.GetPlacements().empty());
    }
}
