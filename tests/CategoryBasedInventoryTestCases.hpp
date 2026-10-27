#pragma once

#include "doctest.h"

#include "GameplayContent/InventoryItem/ItemDef.hpp"
#include "GameplayContent/InventoryItem/ItemInstance.hpp"
#include "Inventory/Examples/CategoryBasedInventories/CategoryBasedInventory.hpp"
#include "Inventory/InventoryChangeTypes.hpp"

#include "Runtime/Function/GameplayFramework/Tags/GameplayTag.h"

namespace
{
    minEngine::GameplayTag MakeCategoryTag(const char* name)
    {
        minEngine::GameplayTag tag;
        tag.SetTagName(name);
        return tag;
    }
}

TEST_CASE("category inventory routes by item category tag")
{
    using namespace minGameplayFramework::InventoryItem;
    using namespace minGameplayFramework::Inventory::CategoryBased;

    const minEngine::GameplayTag weaponTag = MakeCategoryTag("Item.Weapon");
    const minEngine::GameplayTag armorTag = MakeCategoryTag("Item.Armor");
    REQUIRE_FALSE(weaponTag.GetName().empty());
    REQUIRE_FALSE(armorTag.GetName().empty());

    CategoryBasedInventory inventory;
    CategorizedInventory& weapons = inventory.EnsureCategory(weaponTag, 2);
    CategorizedInventory& armors = inventory.EnsureCategory(armorTag, 2);

    ItemDef swordDef;
    swordDef.SetItemCategoryTag(weaponTag);
    swordDef.SetMaxStackCount(1);

    ItemDef helmetDef;
    helmetDef.SetItemCategoryTag(armorTag);
    helmetDef.SetMaxStackCount(1);

    ItemInstance sword;
    sword.SetItemDef(&swordDef);
    sword.SetStackCount(1);
    CHECK(inventory.TryAddItem(sword) == 1);
    CHECK(sword.IsEmpty());
    CHECK_FALSE(weapons.GetSlot(0).IsEmpty());
    CHECK(armors.GetSlot(0).IsEmpty());

    ItemInstance helmet;
    helmet.SetItemDef(&helmetDef);
    helmet.SetStackCount(1);
    CHECK(inventory.TryAddItem(helmet) == 1);
    CHECK_FALSE(armors.GetSlot(0).IsEmpty());
}

TEST_CASE("categorized inventory rejects wrong-category SetSlot and TryAdd")
{
    using namespace minGameplayFramework::InventoryItem;
    using namespace minGameplayFramework::Inventory::CategoryBased;

    const minEngine::GameplayTag weaponTag = MakeCategoryTag("Item.Weapon");
    const minEngine::GameplayTag armorTag = MakeCategoryTag("Item.Armor");
    REQUIRE_FALSE(weaponTag.GetName().empty());
    REQUIRE_FALSE(armorTag.GetName().empty());

    CategorizedInventory weapons;
    weapons.SetCategory(weaponTag);
    weapons.SetCapacity(1);

    ItemDef armorDef;
    armorDef.SetItemCategoryTag(armorTag);
    armorDef.SetMaxStackCount(1);

    ItemInstance helmet;
    helmet.SetItemDef(&armorDef);
    helmet.SetStackCount(1);

    CHECK(weapons.TryAddItem(helmet) == 0);
    CHECK_FALSE(helmet.IsEmpty());
    CHECK_FALSE(weapons.SetSlot(0, helmet));
    CHECK(weapons.GetSlot(0).IsEmpty());
}

TEST_CASE("categorized inventory shrink emits Cleared for vacated trailing slots")
{
    using namespace minGameplayFramework::InventoryItem;
    using namespace minGameplayFramework::Inventory::CategoryBased;
    using minGameplayFramework::Inventory::InventoryChangeReason;

    CategorizedInventory inventory;
    inventory.SetCapacity(2);

    ItemDef stoneDef;
    stoneDef.SetMaxStackCount(1);

    ItemInstance a;
    a.SetItemDef(&stoneDef);
    a.SetStackCount(1);
    inventory.SetSlot(1, std::move(a));

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
