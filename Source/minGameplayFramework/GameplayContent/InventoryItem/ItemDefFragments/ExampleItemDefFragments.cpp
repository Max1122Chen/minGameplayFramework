#include "GameplayContent/InventoryItem/ItemDefFragments/ExampleItemDefFragments.hpp"

namespace minGameplayFramework::InventoryItem
{
    void DurabilityFragment::GrantProperty(ItemInstance& itemInstance) const
    {
        IntInstanceProperty property(kPropertyName, minEngine::GameplayTag{});
        property.SetRange(0, m_MaxDurability);
        property.SetValue(m_MaxDurability);
        itemInstance.AddProperty(std::move(property));
    }
}
