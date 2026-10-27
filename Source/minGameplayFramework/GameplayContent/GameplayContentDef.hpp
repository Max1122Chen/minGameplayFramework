#pragma once

#include "MinGameplayFrameworkAPI.hpp"

#include "Runtime/Core/Object/MEObject.h"
#include "Runtime/Core/Reflection/MEClass.h"
#include "Runtime/Function/GameplayFramework/Tags/GameplayTag.h"

#include <memory>
#include <string>
#include <type_traits>
#include <vector>

namespace minGameplayFramework::GameplayContent
{
    /// @brief 内容片段基类：用组合扩展 ContentDef，避免 Def 继承树爆炸。
    /// @note 迁移时保留「按类型查询片段」即可；身份键见下方 GetFragmentId / GetFragmentIdTag 说明。
    ME_CLASS(Abstract)
    class GameplayContentFragment : public minEngine::MEObject
    {
        ME_GENERATED_BODY()

    public:
        virtual ~GameplayContentFragment() = default;

        /// @brief 字符串身份键（参考实现与 Tag 并存）。
        /// @warning 与 GetFragmentIdTag 同为身份键；迁移到具体项目时应二选一，不要两套都当真源。
        virtual std::string GetFragmentId() const = 0;

        /// @brief GameplayTag 身份键（参考实现与 string 并存）。
        /// @warning GameplayTag 是一项 gameplay 能力，并非所有项目都有；无 Tag 体系时只用 GetFragmentId。
        ///          有 Tag 时也可只用 Tag、去掉 string。迁移时与 GetFragmentId 二选一。
        virtual minEngine::GameplayTag GetFragmentIdTag() const = 0;
    };

    /// @brief 游戏内容定义的抽象基类（物品 / 武器 / 装备等 Schema）。
    /// @note 推荐 Def 只声明值域与要 Grant 的 Instance 属性；可变运行时值放在 Instance。
    ///       运行期改片段字段可行但不提倡。本仓示例用伪数据表达 Def 关系，不强制 DataAsset 管线。
    ME_CLASS(Abstract)
    class GameplayContentDef : public minEngine::MEObject
    {
        ME_GENERATED_BODY()

    public:
        virtual ~GameplayContentDef() = default;

        int GetContentId() const { return m_ContentId; }
        void SetContentId(int contentId) { m_ContentId = contentId; }

        const std::string& GetContentName() const { return m_ContentName; }
        void SetContentName(std::string contentName) { m_ContentName = std::move(contentName); }

        /// @brief 按反射类型取片段；同类型建议最多一个。
        template <typename T>
        T* GetFragment() const
        {
            static_assert(std::is_base_of_v<GameplayContentFragment, T>,
                          "T must derive from GameplayContentFragment");
            return static_cast<T*>(GetFragment(T::StaticClass()));
        }

        template <typename T>
        bool HasFragment() const
        {
            return GetFragment<T>() != nullptr;
        }

        /// @brief 追加片段；同 MEClass 已存在则失败（避免双份打架）。
        bool AddFragment(std::shared_ptr<GameplayContentFragment> fragment);

        /// @brief 按字符串身份查找。与 Tag 重载在参考实现中并存；迁移时与 Tag 版二选一即可。
        GameplayContentFragment* GetFragment(const std::string& fragmentId) const;
        /// @brief 按 Tag 身份查找。无 GameplayTag 的项目可删此重载，只留 string 或 MEClass。
        GameplayContentFragment* GetFragment(const minEngine::GameplayTag& fragmentIdTag) const;
        /// @brief 按反射类型查找（本引擎自然键；迁移到无等价反射时用 string/Tag 之一替代）。
        GameplayContentFragment* GetFragment(const minEngine::Reflection::MEClass* fragmentClass) const;

        const std::vector<std::shared_ptr<GameplayContentFragment>>& GetFragments() const
        {
            return m_Fragments;
        }

    protected:
        ME_PROPERTY()
        int m_ContentId = 0;

        ME_PROPERTY()
        std::string m_ContentName;

        std::vector<std::shared_ptr<GameplayContentFragment>> m_Fragments;
    };
}

#include "GameplayContentDef.gen.h"
