#include "GameplayContent/InventoryItem/ItemInstance.hpp"
#include "GameplayContent/InventoryItem/ItemDef.hpp"

#include <algorithm>
#include <utility>

namespace minGameplayFramework::InventoryItem
{
    void IntInstanceProperty::SetRange(int minValue, int maxValue)
    {
        if (minValue > maxValue)
        {
            std::swap(minValue, maxValue);
        }
        m_MinValue = minValue;
        m_MaxValue = maxValue;
        SetValue(m_Value);
    }

    void IntInstanceProperty::SetValue(int value)
    {
        if (value < m_MinValue)
        {
            value = m_MinValue;
        }
        if (value > m_MaxValue)
        {
            value = m_MaxValue;
        }
        m_Value = value;
    }

    void FloatInstanceProperty::SetRange(float minValue, float maxValue)
    {
        if (!std::isnan(minValue) && !std::isnan(maxValue) && minValue > maxValue)
        {
            std::swap(minValue, maxValue);
        }
        m_MinValue = minValue;
        m_MaxValue = maxValue;
        SetValue(m_Value);
    }

    void FloatInstanceProperty::SetValue(float value)
    {
        // NaN / Inf 输入：有界时仍不强制改写非有限值；仅对有限值做上下限夹取。
        if (std::isfinite(value))
        {
            if (HasMinBound() && value < m_MinValue)
            {
                value = m_MinValue;
            }
            if (HasMaxBound() && value > m_MaxValue)
            {
                value = m_MaxValue;
            }
        }
        m_Value = value;
    }

    ItemInstance::ItemInstance(const ItemInstance& other)
        : m_ItemDef(other.m_ItemDef)
        , m_StackCount(other.m_StackCount)
        , m_IntProperties(other.m_IntProperties)
        , m_FloatProperties(other.m_FloatProperties)
        , m_BoolProperties(other.m_BoolProperties)
    {
        RebuildPropertyMaps();
    }

    ItemInstance& ItemInstance::operator=(const ItemInstance& other)
    {
        if (this == &other)
        {
            return *this;
        }
        m_ItemDef = other.m_ItemDef;
        m_StackCount = other.m_StackCount;
        m_IntProperties = other.m_IntProperties;
        m_FloatProperties = other.m_FloatProperties;
        m_BoolProperties = other.m_BoolProperties;
        RebuildPropertyMaps();
        return *this;
    }

    ItemInstance::ItemInstance(ItemInstance&& other) noexcept
        : m_ItemDef(other.m_ItemDef)
        , m_StackCount(other.m_StackCount)
        , m_IntProperties(std::move(other.m_IntProperties))
        , m_FloatProperties(std::move(other.m_FloatProperties))
        , m_BoolProperties(std::move(other.m_BoolProperties))
    {
        other.m_ItemDef = nullptr;
        other.m_StackCount = 0;
        other.m_NameToPropertyMap.clear();
        RebuildPropertyMaps();
    }

    ItemInstance& ItemInstance::operator=(ItemInstance&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }
        m_ItemDef = other.m_ItemDef;
        m_StackCount = other.m_StackCount;
        m_IntProperties = std::move(other.m_IntProperties);
        m_FloatProperties = std::move(other.m_FloatProperties);
        m_BoolProperties = std::move(other.m_BoolProperties);
        other.m_ItemDef = nullptr;
        other.m_StackCount = 0;
        other.m_NameToPropertyMap.clear();
        RebuildPropertyMaps();
        return *this;
    }

    bool ItemInstance::HasPropertyName(const std::string& propertyName) const
    {
        return m_NameToPropertyMap.find(propertyName) != m_NameToPropertyMap.end();
    }

    bool ItemInstance::AddProperty(IntInstanceProperty property)
    {
        if (HasPropertyName(property.GetName()))
        {
            return false;
        }
        m_IntProperties.push_back(std::move(property));
        RebuildPropertyMaps();
        return true;
    }

    bool ItemInstance::AddProperty(FloatInstanceProperty property)
    {
        if (HasPropertyName(property.GetName()))
        {
            return false;
        }
        m_FloatProperties.push_back(std::move(property));
        RebuildPropertyMaps();
        return true;
    }

    bool ItemInstance::AddProperty(BoolInstanceProperty property)
    {
        if (HasPropertyName(property.GetName()))
        {
            return false;
        }
        m_BoolProperties.push_back(std::move(property));
        RebuildPropertyMaps();
        return true;
    }

    void ItemInstance::RemoveProperty(const std::string& propertyName)
    {
        const auto eraseByName = [&](auto& bucket) {
            bucket.erase(std::remove_if(bucket.begin(), bucket.end(),
                                        [&](const auto& property) {
                                            return property.GetName() == propertyName;
                                        }),
                         bucket.end());
        };
        eraseByName(m_IntProperties);
        eraseByName(m_FloatProperties);
        eraseByName(m_BoolProperties);
        RebuildPropertyMaps();
    }

    const InstancePropertyBase* ItemInstance::GetProperty(const std::string& propertyName) const
    {
        const auto it = m_NameToPropertyMap.find(propertyName);
        return it != m_NameToPropertyMap.end() ? it->second : nullptr;
    }

    InstancePropertyBase* ItemInstance::GetProperty(const std::string& propertyName)
    {
        const auto it = m_NameToPropertyMap.find(propertyName);
        return it != m_NameToPropertyMap.end() ? it->second : nullptr;
    }

    const IntInstanceProperty* ItemInstance::GetIntProperty(const std::string& propertyName) const
    {
        const InstancePropertyBase* property = GetProperty(propertyName);
        if (property == nullptr || property->GetPropertyType() != InstancePropertyType::Int)
        {
            return nullptr;
        }
        return property->AsIntProperty();
    }

    IntInstanceProperty* ItemInstance::GetIntProperty(const std::string& propertyName)
    {
        return const_cast<IntInstanceProperty*>(
            static_cast<const ItemInstance*>(this)->GetIntProperty(propertyName));
    }

    const FloatInstanceProperty* ItemInstance::GetFloatProperty(const std::string& propertyName) const
    {
        const InstancePropertyBase* property = GetProperty(propertyName);
        if (property == nullptr || property->GetPropertyType() != InstancePropertyType::Float)
        {
            return nullptr;
        }
        return property->AsFloatProperty();
    }

    FloatInstanceProperty* ItemInstance::GetFloatProperty(const std::string& propertyName)
    {
        return const_cast<FloatInstanceProperty*>(
            static_cast<const ItemInstance*>(this)->GetFloatProperty(propertyName));
    }

    const BoolInstanceProperty* ItemInstance::GetBoolProperty(const std::string& propertyName) const
    {
        const InstancePropertyBase* property = GetProperty(propertyName);
        if (property == nullptr || property->GetPropertyType() != InstancePropertyType::Bool)
        {
            return nullptr;
        }
        return property->AsBoolProperty();
    }

    BoolInstanceProperty* ItemInstance::GetBoolProperty(const std::string& propertyName)
    {
        return const_cast<BoolInstanceProperty*>(
            static_cast<const ItemInstance*>(this)->GetBoolProperty(propertyName));
    }

    bool ItemInstance::CanStackWith(const ItemInstance& other) const
    {
        if (IsEmpty() || other.IsEmpty())
        {
            return false;
        }
        return m_ItemDef == other.m_ItemDef;
    }

    void ItemInstance::RebuildPropertyMaps()
    {
        m_NameToPropertyMap.clear();
        for (IntInstanceProperty& property : m_IntProperties)
        {
            m_NameToPropertyMap[property.GetName()] = &property;
        }
        for (FloatInstanceProperty& property : m_FloatProperties)
        {
            m_NameToPropertyMap[property.GetName()] = &property;
        }
        for (BoolInstanceProperty& property : m_BoolProperties)
        {
            m_NameToPropertyMap[property.GetName()] = &property;
        }
    }
}
