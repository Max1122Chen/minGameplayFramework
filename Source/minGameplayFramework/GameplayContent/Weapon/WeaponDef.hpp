#pragma once

#include "MinGameplayFrameworkAPI.hpp"
#include "GameplayContent/GameplayContentDef.hpp"
#include "GameplayContent/InventoryItem/ItemDef.hpp"

#include <memory>
#include <string>

namespace minGameplayFramework::Weapon
{
    class WeaponDef;

    /// @brief 挂在 ItemDef 上，引用独立的 WeaponDef（组合，不是 WeaponDef : ItemDef）。
    ME_CLASS()
    class WeaponFragment : public minGameplayFramework::InventoryItem::ItemDefFragmentBase
    {
        ME_GENERATED_BODY()

    public:
        std::string GetFragmentId() const override { return "Weapon"; }
        minEngine::GameplayTag GetFragmentIdTag() const override { return m_FragmentIdTag; }

        const std::string& GetWeaponDefName() const { return m_WeaponDefName; }
        void SetWeaponDefName(std::string name) { m_WeaponDefName = std::move(name); }

        WeaponDef* GetWeaponDef() const { return m_WeaponDef.get(); }
        void SetWeaponDef(std::shared_ptr<WeaponDef> weaponDef) { m_WeaponDef = std::move(weaponDef); }

    private:
        minEngine::GameplayTag m_FragmentIdTag;
        ME_PROPERTY()
        std::string m_WeaponDefName;
        std::shared_ptr<WeaponDef> m_WeaponDef;
    };

    /// @brief 武器内容定义：数据往往不被普通 Item 共享，故独立 ContentDef，由 WeaponFragment 引用。
    ME_CLASS()
    class WeaponDef : public minGameplayFramework::GameplayContent::GameplayContentDef
    {
        ME_GENERATED_BODY()

    public:
        // GameplayAbility* GrantAbility() const; // 配合 GAS，暂未实现
    };
}

#include "WeaponDef.gen.h"
