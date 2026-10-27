#pragma once

#include "doctest.h"

#include "GameplayContent/InventoryItem/ItemDef.hpp"
#include "GameplayContent/InventoryItem/ItemInstance.hpp"
#include "Inventory/Examples/SlotBasedInventories/SlotBasedInventory.hpp"
#include "Inventory/InventoryChangeTypes.hpp"

#include <vector>

TEST_CASE("slot inventory merges then fills empty slots and mutates input stack")
{
    using namespace minGameplayFramework::InventoryItem;
    using namespace minGameplayFramework::Inventory::SlotBased;

    ItemDef stoneDef;
    stoneDef.SetMaxStackCount(64);

    SlotBasedInventory inventory;
    inventory.SetCapacity(3);

    ItemInstance first;
    first.SetItemDef(&stoneDef);
    first.SetStackCount(40);
    CHECK(inventory.TryAddItem(first) == 40);
    CHECK(first.IsEmpty());
    CHECK(inventory.GetSlot(0).GetStackCount() == 40);

    ItemInstance second;
    second.SetItemDef(&stoneDef);
    second.SetStackCount(50);
    CHECK(inventory.TryAddItem(second) == 50);
    CHECK(second.IsEmpty());
    CHECK(inventory.GetSlot(0).GetStackCount() == 64);
    CHECK(inventory.GetSlot(1).GetStackCount() == 26);
}

TEST_CASE("slot inventory rejects overflow beyond capacity")
{
    using namespace minGameplayFramework::InventoryItem;
    using namespace minGameplayFramework::Inventory::SlotBased;

    ItemDef stoneDef;
    stoneDef.SetMaxStackCount(10);

    SlotBasedInventory inventory;
    inventory.SetCapacity(1);

    ItemInstance first;
    first.SetItemDef(&stoneDef);
    first.SetStackCount(10);
    CHECK(inventory.TryAddItem(first) == 10);

    ItemInstance overflow;
    overflow.SetItemDef(&stoneDef);
    overflow.SetStackCount(5);
    CHECK(inventory.TryAddItem(overflow) == 0);
    CHECK(overflow.GetStackCount() == 5);
}

TEST_CASE("minecraft player inventory auto-fill stops before armor region")
{
    using namespace minGameplayFramework::InventoryItem;
    using namespace minGameplayFramework::Inventory::SlotBased;

    ItemDef oneStackDef;
    oneStackDef.SetMaxStackCount(1);

    MinecraftPlayerInventory inventory;
    CHECK(inventory.GetStorageEnd() == MinecraftPlayerInventory::kMainEnd);

    for (int i = 0; i < MinecraftPlayerInventory::kMainEnd; ++i)
    {
        ItemInstance stack;
        stack.SetItemDef(&oneStackDef);
        stack.SetStackCount(1);
        REQUIRE(inventory.TryAddItem(stack) == 1);
    }

    ItemInstance leftover;
    leftover.SetItemDef(&oneStackDef);
    leftover.SetStackCount(1);
    CHECK(inventory.TryAddItem(leftover) == 0);
    CHECK(leftover.GetStackCount() == 1);
    CHECK(inventory.GetSlot(MinecraftPlayerInventory::kArmorBegin).IsEmpty());
}

TEST_CASE("slot inventory SetSlot broadcasts and GetSlot is read only path")
{
    using namespace minGameplayFramework::InventoryItem;
    using namespace minGameplayFramework::Inventory;
    using namespace minGameplayFramework::Inventory::SlotBased;

    ItemDef def;
    def.SetMaxStackCount(10);

    SlotBasedInventory inventory;
    inventory.SetCapacity(2);

    int changedIndex = -1;
    InventoryChangeReason reason = InventoryChangeReason::Added;
    inventory.OnSlotChanged.AddLambda([&](int index, InventoryChangeReason r) {
        changedIndex = index;
        reason = r;
    });

    ItemInstance stack;
    stack.SetItemDef(&def);
    stack.SetStackCount(3);
    inventory.SetSlot(1, std::move(stack));
    CHECK(changedIndex == 1);
    CHECK(reason == InventoryChangeReason::Set);
    CHECK(inventory.GetSlot(1).GetStackCount() == 3);
}

TEST_CASE("slot inventory empty place deep-copies instance properties")
{
    using namespace minGameplayFramework::InventoryItem;
    using namespace minGameplayFramework::Inventory::SlotBased;

    ItemDef def;
    def.SetMaxStackCount(5);

    SlotBasedInventory inventory;
    inventory.SetCapacity(1);

    ItemInstance stack;
    stack.SetItemDef(&def);
    stack.SetStackCount(2);
    IntInstanceProperty durability("Durability", minEngine::GameplayTag{});
    durability.SetRange(0, 100);
    durability.SetValue(77);
    stack.AddProperty(std::move(durability));

    REQUIRE(inventory.TryAddItem(stack) == 2);
    const IntInstanceProperty* copied = inventory.GetSlot(0).GetIntProperty("Durability");
    REQUIRE(copied != nullptr);
    CHECK(copied->GetValue() == 77);
}

TEST_CASE("slot inventory shrink compacts into front then evicts remainder")
{
    using namespace minGameplayFramework::InventoryItem;
    using namespace minGameplayFramework::Inventory;
    using namespace minGameplayFramework::Inventory::SlotBased;

    ItemDef aDef;
    aDef.SetMaxStackCount(1);
    ItemDef bDef;
    bDef.SetMaxStackCount(1);
    ItemDef cDef;
    cDef.SetMaxStackCount(1);

    SlotBasedInventory inventory;
    inventory.SetCapacity(3);

    ItemInstance a;
    a.SetItemDef(&aDef);
    a.SetStackCount(1);
    inventory.SetSlot(0, std::move(a));

    // Leave slot 1 empty; put items in trailing slots.
    ItemInstance b;
    b.SetItemDef(&bDef);
    b.SetStackCount(1);
    inventory.SetSlot(2, std::move(b));

    ItemInstance c;
    c.SetItemDef(&cDef);
    c.SetStackCount(1);
    // Need capacity 4 first to have two trailing items when shrinking to 1.
    inventory.SetCapacity(4);
    inventory.SetSlot(3, std::move(c));

    // Layout: [A][ ][B][C], shrink to 2 → B fills slot1, C evicted (A stays, one empty for B).
    std::vector<ItemInstance> evicted;
    int oldCap = -1;
    int newCap = -1;
    inventory.OnItemsEvicted.AddLambda([&](const InventoryEvictionBatch& batch) {
        evicted = batch.Items;
    });
    inventory.OnCapacityChanged.AddLambda([&](int oldC, int newC) {
        oldCap = oldC;
        newCap = newC;
    });

    inventory.SetCapacity(2);

    CHECK(oldCap == 4);
    CHECK(newCap == 2);
    CHECK(inventory.GetCapacity() == 2);
    CHECK_FALSE(inventory.GetSlot(0).IsEmpty());
    CHECK_FALSE(inventory.GetSlot(1).IsEmpty());
    REQUIRE(evicted.size() == 1);
    CHECK(evicted[0].GetItemDef() == &cDef);
}

TEST_CASE("slot inventory TryAddTo and TryRemoveFrom")
{
    using namespace minGameplayFramework::InventoryItem;
    using namespace minGameplayFramework::Inventory::SlotBased;

    ItemDef def;
    def.SetMaxStackCount(10);

    SlotBasedInventory inventory;
    inventory.SetCapacity(2);

    ItemInstance stack;
    stack.SetItemDef(&def);
    stack.SetStackCount(4);
    CHECK(inventory.TryAddTo(0, stack) == 4);
    CHECK(inventory.GetSlot(0).GetStackCount() == 4);

    ItemInstance taken = inventory.TryRemoveFrom(0, 2);
    CHECK(taken.GetStackCount() == 2);
    CHECK(inventory.GetSlot(0).GetStackCount() == 2);
}

TEST_CASE("slot inventory shrink emits Cleared for vacated trailing slots")
{
    using namespace minGameplayFramework::InventoryItem;
    using namespace minGameplayFramework::Inventory::SlotBased;
    using minGameplayFramework::Inventory::InventoryChangeReason;

    ItemDef def;
    def.SetMaxStackCount(1);

    SlotBasedInventory inventory;
    inventory.SetCapacity(2);

    ItemInstance item;
    item.SetItemDef(&def);
    item.SetStackCount(1);
    inventory.SetSlot(1, std::move(item));

    bool sawClearedOnOne = false;
    inventory.OnSlotChanged.AddLambda([&](int index, InventoryChangeReason reason) {
        if (index == 1 && reason == InventoryChangeReason::Cleared)
        {
            sawClearedOnOne = true;
        }
    });

    inventory.SetCapacity(1);
    CHECK(sawClearedOnOne);
    CHECK_FALSE(inventory.GetSlot(0).IsEmpty());
}

TEST_CASE("slot inventory TryRemoveItem mutates request stack remainder")
{
    using namespace minGameplayFramework::InventoryItem;
    using namespace minGameplayFramework::Inventory::SlotBased;

    ItemDef def;
    def.SetMaxStackCount(10);

    SlotBasedInventory inventory;
    inventory.SetCapacity(1);

    ItemInstance add;
    add.SetItemDef(&def);
    add.SetStackCount(5);
    CHECK(inventory.TryAddItem(add) == 5);

    ItemInstance request;
    request.SetItemDef(&def);
    request.SetStackCount(8);
    CHECK(inventory.TryRemoveItem(request) == 5);
    CHECK(request.GetStackCount() == 3);
    CHECK(request.GetItemDef() == &def);
    CHECK(inventory.GetSlot(0).IsEmpty());
}
