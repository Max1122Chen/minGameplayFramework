#pragma once

#include "doctest.h"

#include "GameplayContent/InventoryItem/ItemDef.hpp"
#include "GameplayContent/InventoryItem/ItemDefFragments/ExampleItemDefFragments.hpp"
#include "GameplayContent/InventoryItem/ItemInstance.hpp"

#include <climits>
#include <cmath>
#include <limits>

TEST_CASE("item instance stores properties in typed buckets")
{
    using namespace minGameplayFramework::InventoryItem;

    ItemInstance instance;

    IntInstanceProperty durability("Durability", minEngine::GameplayTag{});
    durability.SetRange(0, 100);
    durability.SetValue(80);
    CHECK(instance.AddProperty(std::move(durability)));

    FloatInstanceProperty weight("Weight", minEngine::GameplayTag{});
    weight.SetRange(0.0f, 50.0f);
    weight.SetValue(12.5f);
    CHECK(instance.AddProperty(std::move(weight)));

    BoolInstanceProperty identified("Identified", minEngine::GameplayTag{});
    identified.SetValue(true);
    CHECK(instance.AddProperty(std::move(identified)));

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

TEST_CASE("item instance int property defaults to unrestricted range")
{
    using namespace minGameplayFramework::InventoryItem;

    IntInstanceProperty property("Score", minEngine::GameplayTag{});
    CHECK(property.GetMinValue() == INT_MIN);
    CHECK(property.GetMaxValue() == INT_MAX);
    property.SetValue(-100);
    CHECK(property.GetValue() == -100);
    property.SetValue(100000);
    CHECK(property.GetValue() == 100000);
}

TEST_CASE("item instance int property clamps to range and swaps inverted bounds")
{
    using namespace minGameplayFramework::InventoryItem;

    IntInstanceProperty property("Durability", minEngine::GameplayTag{});
    property.SetRange(0, 10);
    property.SetValue(100);
    CHECK(property.GetValue() == 10);
    property.SetValue(-5);
    CHECK(property.GetValue() == 0);

    property.SetRange(20, 5);
    CHECK(property.GetMinValue() == 5);
    CHECK(property.GetMaxValue() == 20);
    CHECK(property.GetValue() == 5);
}

TEST_CASE("item instance float property defaults to NaN unbounded range")
{
    using namespace minGameplayFramework::InventoryItem;

    FloatInstanceProperty property("Weight", minEngine::GameplayTag{});
    CHECK_FALSE(property.HasMinBound());
    CHECK_FALSE(property.HasMaxBound());
    CHECK(std::isnan(property.GetMinValue()));
    CHECK(std::isnan(property.GetMaxValue()));
    property.SetValue(-12.5f);
    CHECK(property.GetValue() == doctest::Approx(-12.5f));
    property.SetValue(1.0e20f);
    CHECK(property.GetValue() == doctest::Approx(1.0e20f));
}

TEST_CASE("item instance float property clamps only finite bounded sides")
{
    using namespace minGameplayFramework::InventoryItem;

    FloatInstanceProperty property("Charge", minEngine::GameplayTag{});
    property.SetRange(0.0f, 1.0f);
    property.SetValue(2.0f);
    CHECK(property.GetValue() == doctest::Approx(1.0f));
    property.SetValue(-1.0f);
    CHECK(property.GetValue() == doctest::Approx(0.0f));

    property.SetRange(std::numeric_limits<float>::quiet_NaN(), 10.0f);
    property.SetValue(-100.0f);
    CHECK(property.GetValue() == doctest::Approx(-100.0f));
    property.SetValue(50.0f);
    CHECK(property.GetValue() == doctest::Approx(10.0f));
}

TEST_CASE("item instance rejects duplicate property names across typed buckets")
{
    using namespace minGameplayFramework::InventoryItem;

    ItemInstance instance;
    CHECK(instance.AddProperty(IntInstanceProperty("Shared", minEngine::GameplayTag{})));
    CHECK_FALSE(instance.AddProperty(FloatInstanceProperty("Shared", minEngine::GameplayTag{})));
    CHECK_FALSE(instance.AddProperty(BoolInstanceProperty("Shared", minEngine::GameplayTag{})));
    CHECK_FALSE(instance.AddProperty(IntInstanceProperty("Shared", minEngine::GameplayTag{})));

    CHECK(instance.GetIntProperties().size() == 1);
    CHECK(instance.GetFloatProperties().empty());
    CHECK(instance.GetBoolProperties().empty());
    CHECK(instance.GetIntProperty("Shared") != nullptr);
    CHECK(instance.GetFloatProperty("Shared") == nullptr);
}

TEST_CASE("item instance copy deep-copies typed buckets and keeps lookup")
{
    using namespace minGameplayFramework::InventoryItem;

    ItemInstance original;
    IntInstanceProperty property("Durability", minEngine::GameplayTag{});
    property.SetRange(0, 50);
    property.SetValue(25);
    CHECK(original.AddProperty(std::move(property)));

    ItemInstance copy = original;
    IntInstanceProperty* copied = copy.GetIntProperty("Durability");
    REQUIRE(copied != nullptr);
    copied->SetValue(10);

    const IntInstanceProperty* source = original.GetIntProperty("Durability");
    REQUIRE(source != nullptr);
    CHECK(source->GetValue() == 25);
    CHECK(copied->GetValue() == 10);
}

TEST_CASE("item instance move keeps property lookup valid")
{
    using namespace minGameplayFramework::InventoryItem;

    ItemInstance original;
    IntInstanceProperty property("Durability", minEngine::GameplayTag{});
    property.SetRange(0, 50);
    property.SetValue(25);
    CHECK(original.AddProperty(std::move(property)));

    ItemInstance moved = std::move(original);
    const IntInstanceProperty* durability = moved.GetIntProperty("Durability");
    REQUIRE(durability != nullptr);
    CHECK(durability->GetValue() == 25);
    CHECK(original.GetIntProperty("Durability") == nullptr);
}

TEST_CASE("item instance remove property clears bucket entry")
{
    using namespace minGameplayFramework::InventoryItem;

    ItemInstance instance;
    CHECK(instance.AddProperty(IntInstanceProperty("Durability", minEngine::GameplayTag{})));
    CHECK(instance.AddProperty(BoolInstanceProperty("Identified", minEngine::GameplayTag{})));
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

    // Same def with different instance properties still stacks under current contract.
    IntInstanceProperty durability("Durability", minEngine::GameplayTag{});
    durability.SetRange(0, 100);
    durability.SetValue(10);
    CHECK(a.AddProperty(std::move(durability)));
    IntInstanceProperty otherDurability("Durability", minEngine::GameplayTag{});
    otherDurability.SetRange(0, 100);
    otherDurability.SetValue(90);
    CHECK(b.AddProperty(std::move(otherDurability)));
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
