#include "GameplayContent/InventoryItem/ItemInstance.hpp"
#include "GameplayContent/InventoryItem/ItemDef.hpp"

#include <algorithm>

namespace minGameplayFramework::InventoryItem
{
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

    void FloatInstanceProperty::SetValue(float value)
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

    void ItemInstance::AddProperty(IntInstanceProperty property)
    {
        m_IntProperties.push_back(std::move(property));
        RebuildPropertyMaps();
    }

    void ItemInstance::AddProperty(FloatInstanceProperty property)
    {
        m_FloatProperties.push_back(std::move(property));
        RebuildPropertyMaps();
    }

    void ItemInstance::AddProperty(BoolInstanceProperty property)
    {
        m_BoolProperties.push_back(std::move(property));
        RebuildPropertyMaps();
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
