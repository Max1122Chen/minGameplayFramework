#pragma once

#include "doctest.h"

#include "GameplayContent/InventoryItem/ItemDef.hpp"
#include "GameplayContent/InventoryItem/ItemDefFragments/ExampleItemDefFragments.hpp"
#include "GameplayContent/InventoryItem/ItemInstance.hpp"

TEST_CASE("item instance stores properties in typed buckets")
{
    using namespace minGameplayFramework::InventoryItem;

    ItemInstance instance;

    IntInstanceProperty durability("Durability", minEngine::GameplayTag{});
    durability.SetRange(0, 100);
    durability.SetValue(80);
    instance.AddProperty(std::move(durability));

    FloatInstanceProperty weight("Weight", minEngine::GameplayTag{});
    weight.SetRange(0.0f, 50.0f);
    weight.SetValue(12.5f);
    instance.AddProperty(std::move(weight));

    BoolInstanceProperty identified("Identified", minEngine::GameplayTag{});
    identified.SetValue(true);
    instance.AddProperty(std::move(identified));

    CHECK(instance.GetIntProperties().size() == 1);
    CHECK(instance.GetFloatProperties().size() == 1);
    CHECK(instance.GetBoolProperties().size() == 1);

    const IntInstanceProperty* durabilityProperty = instance.GetIntProperty("Durability");
    REQUIRE(durabilityProperty != nullptr);
    CHECK(durabilityProperty->GetValue() == 80);
    CHECK(durabilityProperty->GetMaxValue() == 100);

    const FloatInstanceProperty* weightProperty = instance.GetFloatProperty("Weight");
    REQUIRE(weightProperty != nullptr);
    CHECK(weightProperty->GetValue() == doctest::Approx(12.5f));

    const BoolInstanceProperty* identifiedProperty = instance.GetBoolProperty("Identified");
    REQUIRE(identifiedProperty != nullptr);
    CHECK(identifiedProperty->GetValue() == true);
}

TEST_CASE("item instance int property clamps to range")
{
    using namespace minGameplayFramework::InventoryItem;

    IntInstanceProperty property("Durability", minEngine::GameplayTag{});
    property.SetRange(0, 10);
    property.SetValue(100);
    CHECK(property.GetValue() == 10);
    property.SetValue(-5);
    CHECK(property.GetValue() == 0);
}

TEST_CASE("item instance copy deep-copies typed buckets and keeps lookup")
{
    using namespace minGameplayFramework::InventoryItem;

    ItemInstance original;
    IntInstanceProperty property("Durability", minEngine::GameplayTag{});
    property.SetRange(0, 50);
    property.SetValue(25);
    original.AddProperty(std::move(property));

    ItemInstance copy = original;
    IntInstanceProperty* copied = copy.GetIntProperty("Durability");
    REQUIRE(copied != nullptr);
    copied->SetValue(10);

    const IntInstanceProperty* source = original.GetIntProperty("Durability");
    REQUIRE(source != nullptr);
    CHECK(source->GetValue() == 25);
    CHECK(copied->GetValue() == 10);
}

TEST_CASE("item instance remove property clears bucket entry")
{
    using namespace minGameplayFramework::InventoryItem;

    ItemInstance instance;
    instance.AddProperty(IntInstanceProperty("Durability", minEngine::GameplayTag{}));
    instance.AddProperty(BoolInstanceProperty("Identified", minEngine::GameplayTag{}));
    instance.RemoveProperty("Durability");

    CHECK(instance.GetIntProperty("Durability") == nullptr);
    CHECK(instance.GetIntProperties().empty());
    CHECK(instance.GetBoolProperty("Identified") != nullptr);
}

TEST_CASE("item instance can stack only with same def and non-empty stacks")
{
    using namespace minGameplayFramework::InventoryItem;

    ItemDef stoneDef;
    ItemDef woodDef;

    ItemInstance a;
    a.SetItemDef(&stoneDef);
    a.SetStackCount(3);

    ItemInstance b;
    b.SetItemDef(&stoneDef);
    b.SetStackCount(2);
    CHECK(a.CanStackWith(b));

    ItemInstance c;
    c.SetItemDef(&woodDef);
    c.SetStackCount(2);
    CHECK_FALSE(a.CanStackWith(c));

    ItemInstance empty;
    empty.SetItemDef(&stoneDef);
    empty.SetStackCount(0);
    CHECK_FALSE(a.CanStackWith(empty));
}

TEST_CASE("durability fragment grants int property onto instance")
{
    using namespace minGameplayFramework::InventoryItem;

    DurabilityFragment fragment;
    fragment.SetMaxDurability(200);

    ItemInstance instance;
    fragment.GrantProperty(instance);

    const IntInstanceProperty* durability = instance.GetIntProperty(DurabilityFragment::kPropertyName);
    REQUIRE(durability != nullptr);
    CHECK(durability->GetValue() == 200);
    CHECK(durability->GetMaxValue() == 200);
}
