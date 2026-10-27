#include "GameplayContent/GameplayContentDef.hpp"

namespace minGameplayFramework::GameplayContent
{
    bool GameplayContentDef::AddFragment(std::shared_ptr<GameplayContentFragment> fragment)
    {
        if (fragment == nullptr)
        {
            return false;
        }

        const minEngine::Reflection::MEClass* fragmentClass = fragment->GetClass();
        if (fragmentClass != nullptr && GetFragment(fragmentClass) != nullptr)
        {
            return false;
        }

        m_Fragments.push_back(std::move(fragment));
        return true;
    }

    GameplayContentFragment* GameplayContentDef::GetFragment(const std::string& fragmentId) const
    {
        for (const auto& fragment : m_Fragments)
        {
            if (fragment != nullptr && fragment->GetFragmentId() == fragmentId)
            {
                return fragment.get();
            }
        }
        return nullptr;
    }

    GameplayContentFragment* GameplayContentDef::GetFragment(const minEngine::GameplayTag& fragmentIdTag) const
    {
        for (const auto& fragment : m_Fragments)
        {
            if (fragment != nullptr && fragment->GetFragmentIdTag() == fragmentIdTag)
            {
                return fragment.get();
            }
        }
        return nullptr;
    }

    GameplayContentFragment* GameplayContentDef::GetFragment(
        const minEngine::Reflection::MEClass* fragmentClass) const
    {
        if (fragmentClass == nullptr)
        {
            return nullptr;
        }

        for (const auto& fragment : m_Fragments)
        {
            if (fragment != nullptr && fragment->GetClass() == fragmentClass)
            {
                return fragment.get();
            }
        }
        return nullptr;
    }
}
