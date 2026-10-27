#pragma once

#include "MinGameplayFrameworkAPI.hpp"
#include "GameplayContent/GameplayContentDef.hpp"
#include "GameplayContent/InventoryItem/ItemDef.hpp"

#include <memory>
#include <string>

namespace minGameplayFramework::Equipment
{
    class EquipmentDef;

    /// @brief 挂在 ItemDef 上，引用独立的 EquipmentDef（头盔/胸甲等与 Item 展示解耦）。
    ME_CLASS()
    class EquipmentFragment : public minGameplayFramework::InventoryItem::ItemDefFragmentBase
    {
        ME_GENERATED_BODY()

    public:
        std::string GetFragmentId() const override { return "Equipment"; }
        minEngine::GameplayTag GetFragmentIdTag() const override { return m_FragmentIdTag; }

        const std::string& GetEquipmentDefName() const { return m_EquipmentDefName; }
        void SetEquipmentDefName(std::string name) { m_EquipmentDefName = std::move(name); }

        EquipmentDef* GetEquipmentDef() const { return m_EquipmentDef.get(); }
        void SetEquipmentDef(std::shared_ptr<EquipmentDef> equipmentDef)
        {
            m_EquipmentDef = std::move(equipmentDef);
        }

    private:
        minEngine::GameplayTag m_FragmentIdTag;
        ME_PROPERTY()
        std::string m_EquipmentDefName;
        std::shared_ptr<EquipmentDef> m_EquipmentDef;
    };

    /// @brief 装备内容定义（独立 Catalog），不继承 ItemDef。
    ME_CLASS()
    class EquipmentDef : public minGameplayFramework::GameplayContent::GameplayContentDef
    {
        ME_GENERATED_BODY()
    };
}

#include "EquipmentDef.gen.h"
