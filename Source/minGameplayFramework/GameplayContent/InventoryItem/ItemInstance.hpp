#pragma once

#include "MinGameplayFrameworkAPI.hpp"

#include "Runtime/Core/Assert/Assert.h"
#include "Runtime/Function/GameplayFramework/Tags/GameplayTag.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace minGameplayFramework::InventoryItem
{
    class ItemDef;
    class IntInstanceProperty;
    class FloatInstanceProperty;
    class BoolInstanceProperty;

    /// @brief Instance 属性值类型。常用三种优先；其余按需扩展。
    enum class InstancePropertyType
    {
        Int,
        Float,
        Bool,
        String,
        Vector2,
        Vector3,
        Vector4,
        ObjectRef,
    };

    /// @brief Def 赋予 Instance 的可选属性袋基类（值语义；按类型分桶存储以保证同类型连续）。
    /// @warning 身份键 Name 与 NameTag 在本参考实现中并存；迁移到具体项目时应二选一。
    ///          GameplayTag 本身是一项 gameplay 能力，并非所有引擎/项目都有，无 Tag 时只用 string 即可。
    class InstancePropertyBase
    {
    public:
        virtual ~InstancePropertyBase() = default;

        const std::string& GetName() const { return m_Name; }
        const minEngine::GameplayTag& GetNameTag() const { return m_NameTag; }
        virtual InstancePropertyType GetPropertyType() const = 0;

        IntInstanceProperty* AsIntProperty();
        FloatInstanceProperty* AsFloatProperty();
        BoolInstanceProperty* AsBoolProperty();
        const IntInstanceProperty* AsIntProperty() const;
        const FloatInstanceProperty* AsFloatProperty() const;
        const BoolInstanceProperty* AsBoolProperty() const;

    protected:
        InstancePropertyBase(std::string name, minEngine::GameplayTag nameTag)
            : m_Name(std::move(name))
            , m_NameTag(std::move(nameTag))
        {
        }

        std::string m_Name;
        minEngine::GameplayTag m_NameTag;
    };

    class IntInstanceProperty : public InstancePropertyBase
    {
    public:
        IntInstanceProperty(std::string name, minEngine::GameplayTag nameTag)
            : InstancePropertyBase(std::move(name), std::move(nameTag))
        {
        }

        InstancePropertyType GetPropertyType() const override { return InstancePropertyType::Int; }

        void SetRange(int minValue, int maxValue)
        {
            m_MinValue = minValue;
            m_MaxValue = maxValue;
        }

        int GetMinValue() const { return m_MinValue; }
        int GetMaxValue() const { return m_MaxValue; }
        int GetValue() const { return m_Value; }
        void SetValue(int value);

    private:
        int m_MinValue = 0;
        int m_MaxValue = 0;
        int m_Value = 0;
    };

    class FloatInstanceProperty : public InstancePropertyBase
    {
    public:
        FloatInstanceProperty(std::string name, minEngine::GameplayTag nameTag)
            : InstancePropertyBase(std::move(name), std::move(nameTag))
        {
        }

        InstancePropertyType GetPropertyType() const override { return InstancePropertyType::Float; }

        void SetRange(float minValue, float maxValue)
        {
            m_MinValue = minValue;
            m_MaxValue = maxValue;
        }

        float GetMinValue() const { return m_MinValue; }
        float GetMaxValue() const { return m_MaxValue; }
        float GetValue() const { return m_Value; }
        void SetValue(float value);

    private:
        float m_MinValue = 0.0f;
        float m_MaxValue = 0.0f;
        float m_Value = 0.0f;
    };

    class BoolInstanceProperty : public InstancePropertyBase
    {
    public:
        BoolInstanceProperty(std::string name, minEngine::GameplayTag nameTag)
            : InstancePropertyBase(std::move(name), std::move(nameTag))
        {
        }

        InstancePropertyType GetPropertyType() const override { return InstancePropertyType::Bool; }

        bool GetValue() const { return m_Value; }
        void SetValue(bool value) { m_Value = value; }

    private:
        bool m_Value = false;
    };

    inline IntInstanceProperty* InstancePropertyBase::AsIntProperty()
    {
        ME_ASSERT(GetPropertyType() == InstancePropertyType::Int, "Invalid property type");
        return static_cast<IntInstanceProperty*>(this);
    }

    inline FloatInstanceProperty* InstancePropertyBase::AsFloatProperty()
    {
        ME_ASSERT(GetPropertyType() == InstancePropertyType::Float, "Invalid property type");
        return static_cast<FloatInstanceProperty*>(this);
    }

    inline BoolInstanceProperty* InstancePropertyBase::AsBoolProperty()
    {
        ME_ASSERT(GetPropertyType() == InstancePropertyType::Bool, "Invalid property type");
        return static_cast<BoolInstanceProperty*>(this);
    }

    inline const IntInstanceProperty* InstancePropertyBase::AsIntProperty() const
    {
        ME_ASSERT(GetPropertyType() == InstancePropertyType::Int, "Invalid property type");
        return static_cast<const IntInstanceProperty*>(this);
    }

    inline const FloatInstanceProperty* InstancePropertyBase::AsFloatProperty() const
    {
        ME_ASSERT(GetPropertyType() == InstancePropertyType::Float, "Invalid property type");
        return static_cast<const FloatInstanceProperty*>(this);
    }

    inline const BoolInstanceProperty* InstancePropertyBase::AsBoolProperty() const
    {
        ME_ASSERT(GetPropertyType() == InstancePropertyType::Bool, "Invalid property type");
        return static_cast<const BoolInstanceProperty*>(this);
    }

    /// @brief 物品运行时实例：Def 引用 + 堆叠 + 按类型分桶的属性袋（同类型连续，便于缓存）。
    class ItemInstance
    {
    public:
        ItemInstance() = default;
        ItemInstance(const ItemInstance& other);
        ItemInstance& operator=(const ItemInstance& other);
        ItemInstance(ItemInstance&&) noexcept = default;
        ItemInstance& operator=(ItemInstance&&) noexcept = default;

        const ItemDef* GetItemDef() const { return m_ItemDef; }
        void SetItemDef(ItemDef* itemDef) { m_ItemDef = itemDef; }

        int GetStackCount() const { return m_StackCount; }
        void SetStackCount(int stackCount) { m_StackCount = stackCount < 0 ? 0 : stackCount; }

        bool IsEmpty() const { return m_ItemDef == nullptr || m_StackCount <= 0; }

        /// @brief 通常由片段 GrantProperty 在创建实例时调用；按值写入对应类型桶。
        void AddProperty(IntInstanceProperty property);
        void AddProperty(FloatInstanceProperty property);
        void AddProperty(BoolInstanceProperty property);

        void RemoveProperty(const std::string& propertyName);

        const InstancePropertyBase* GetProperty(const std::string& propertyName) const;
        InstancePropertyBase* GetProperty(const std::string& propertyName);

        const IntInstanceProperty* GetIntProperty(const std::string& propertyName) const;
        IntInstanceProperty* GetIntProperty(const std::string& propertyName);
        const FloatInstanceProperty* GetFloatProperty(const std::string& propertyName) const;
        FloatInstanceProperty* GetFloatProperty(const std::string& propertyName);
        const BoolInstanceProperty* GetBoolProperty(const std::string& propertyName) const;
        BoolInstanceProperty* GetBoolProperty(const std::string& propertyName);

        const std::vector<IntInstanceProperty>& GetIntProperties() const { return m_IntProperties; }
        const std::vector<FloatInstanceProperty>& GetFloatProperties() const { return m_FloatProperties; }
        const std::vector<BoolInstanceProperty>& GetBoolProperties() const { return m_BoolProperties; }

        bool CanStackWith(const ItemInstance& other) const;

    private:
        ItemDef* m_ItemDef = nullptr;
        int m_StackCount = 0;

        // 按类型分桶：同类型元素在内存中连续。
        std::vector<IntInstanceProperty> m_IntProperties;
        std::vector<FloatInstanceProperty> m_FloatProperties;
        std::vector<BoolInstanceProperty> m_BoolProperties;

        // 运行时快捷索引；指向各桶内元素，增删后必须 Rebuild。
        std::unordered_map<std::string, InstancePropertyBase*> m_NameToPropertyMap;

        void RebuildPropertyMaps();
    };
}
