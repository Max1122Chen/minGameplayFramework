#pragma once

#include "MinGameplayFrameworkAPI.hpp"

#include "Runtime/Core/Assert/Assert.h"
#include "Runtime/Function/GameplayFramework/Tags/GameplayTag.h"

#include <climits>
#include <cmath>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

namespace minGameplayFramework::InventoryItem
{
    class ItemDef;
    class IntInstanceProperty;
    class FloatInstanceProperty;
    class BoolInstanceProperty;

    /// @brief Instance 属性值类型。
    /// @note 本参考实现仅落地 Int / Float / Bool；其余枚举值为占位，无存储与公开 API。
    enum class InstancePropertyType
    {
        Int,
        Float,
        Bool,
        String,    // 未实现 / not implemented
        Vector2,   // 未实现 / not implemented
        Vector3,   // 未实现 / not implemented
        Vector4,   // 未实现 / not implemented
        ObjectRef, // 未实现 / not implemented
    };

    /// @brief Def 赋予 Instance 的可选属性袋基类（值语义；按类型分桶存储以保证同类型连续）。
    /// @warning 身份键 Name 与 NameTag 在本参考实现中并存；迁移到具体项目时应二选一。
    ///          GameplayTag 本身是一项 gameplay 能力，并非所有引擎/项目都有，无 Tag 时只用 string 即可。
    /// @warning 属性名在同一 ItemInstance 内跨类型桶全局唯一；重复名的 AddProperty 会失败。
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

    /// @brief 整型 Instance 属性。默认范围为 [INT_MIN, INT_MAX]，表示不限制。
    class IntInstanceProperty : public InstancePropertyBase
    {
    public:
        IntInstanceProperty(std::string name, minEngine::GameplayTag nameTag)
            : InstancePropertyBase(std::move(name), std::move(nameTag))
        {
        }

        InstancePropertyType GetPropertyType() const override { return InstancePropertyType::Int; }

        /// @brief 设置闭区间范围；若 minValue > maxValue 则交换两端。
        void SetRange(int minValue, int maxValue);

        int GetMinValue() const { return m_MinValue; }
        int GetMaxValue() const { return m_MaxValue; }
        int GetValue() const { return m_Value; }
        void SetValue(int value);

    private:
        int m_MinValue = INT_MIN;
        int m_MaxValue = INT_MAX;
        int m_Value = 0;
    };

    /// @brief 浮点 Instance 属性。默认 min/max 为 NaN，表示该侧无限制。
    class FloatInstanceProperty : public InstancePropertyBase
    {
    public:
        FloatInstanceProperty(std::string name, minEngine::GameplayTag nameTag)
            : InstancePropertyBase(std::move(name), std::move(nameTag))
        {
        }

        InstancePropertyType GetPropertyType() const override { return InstancePropertyType::Float; }

        /// @brief 设置范围；任一端为 NaN 表示该侧无限制。两端皆有限且 min>max 时交换。
        void SetRange(float minValue, float maxValue);

        float GetMinValue() const { return m_MinValue; }
        float GetMaxValue() const { return m_MaxValue; }
        bool HasMinBound() const { return !std::isnan(m_MinValue); }
        bool HasMaxBound() const { return !std::isnan(m_MaxValue); }

        float GetValue() const { return m_Value; }
        void SetValue(float value);

    private:
        float m_MinValue = std::numeric_limits<float>::quiet_NaN();
        float m_MaxValue = std::numeric_limits<float>::quiet_NaN();
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
    /// @warning m_ItemDef 为非拥有原始指针。调用方须保证 Def 寿命覆盖所有仍引用它的 Instance
    ///          （典型：Registry / Catalog 持有 shared_ptr，进程内常驻）。本参考实现不处理热重载卸载顺序。
    /// @note CanStackWith 仅比较 ItemDef 指针是否相同（且双方非空堆）。不比较 Instance 属性；
    ///       当前示例里带独立属性的物品 maxStack 多为 1。若将来可堆叠且属性不同，需另行扩展兼容规则。
    class ItemInstance
    {
    public:
        ItemInstance() = default;
        ItemInstance(const ItemInstance& other);
        ItemInstance& operator=(const ItemInstance& other);
        ItemInstance(ItemInstance&& other) noexcept;
        ItemInstance& operator=(ItemInstance&& other) noexcept;

        const ItemDef* GetItemDef() const { return m_ItemDef; }
        void SetItemDef(ItemDef* itemDef) { m_ItemDef = itemDef; }

        int GetStackCount() const { return m_StackCount; }
        void SetStackCount(int stackCount) { m_StackCount = stackCount < 0 ? 0 : stackCount; }

        bool IsEmpty() const { return m_ItemDef == nullptr || m_StackCount <= 0; }

        /// @brief 按值写入对应类型桶；名称已存在（含跨类型）时失败并返回 false。
        /// @return 是否成功添加。
        bool AddProperty(IntInstanceProperty property);
        bool AddProperty(FloatInstanceProperty property);
        bool AddProperty(BoolInstanceProperty property);

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

        /// @brief 同 Def 指针且双方非空堆则可堆叠；不比较属性袋。
        bool CanStackWith(const ItemInstance& other) const;

    private:
        ItemDef* m_ItemDef = nullptr;
        int m_StackCount = 0;

        // 按类型分桶：同类型元素在内存中连续。
        std::vector<IntInstanceProperty> m_IntProperties;
        std::vector<FloatInstanceProperty> m_FloatProperties;
        std::vector<BoolInstanceProperty> m_BoolProperties;

        // 运行时快捷索引；指向各桶内元素，增删/复制/移动后必须 Rebuild。
        std::unordered_map<std::string, InstancePropertyBase*> m_NameToPropertyMap;

        void RebuildPropertyMaps();
        bool HasPropertyName(const std::string& propertyName) const;
    };
}
