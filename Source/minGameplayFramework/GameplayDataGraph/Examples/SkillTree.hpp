#pragma once

#include "MinGameplayFrameworkAPI.hpp"

#include "Runtime/Core/Delegates/MulticastDelegate.h"
#include "Runtime/Core/Object/MEObject.h"
#include "Runtime/Function/GameplayFramework/Tags/GameplayTag.h"
#include "Runtime/Resource/DataAsset.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace minGameplayFramework::SkillsTree
{
    /// @brief 简单的技能内容示意：只承担技能本身的业务，不表达技能之间的依赖关系。
    /// @note 依赖关系由技能树资产定义，以遵守单一职责；正式项目中可改为更完整的效果/GAS 描述。
    ME_STRUCT()
    struct Skill
    {
        ME_GENERATED_BODY()

        const std::string& GetName() const { return m_Name; }
        void SetName(std::string name) { m_Name = std::move(name); }

        const minEngine::GameplayTag& GetSkillTag() const { return m_SkillTag; }
        void SetSkillTag(minEngine::GameplayTag tag) { m_SkillTag = std::move(tag); }

        const std::vector<minEngine::GameplayTag>& GetEffects() const { return m_Effects; }
        void SetEffects(std::vector<minEngine::GameplayTag> effects) { m_Effects = std::move(effects); }

    private:
        ME_PROPERTY()
        std::string m_Name;

        ME_PROPERTY()
        minEngine::GameplayTag m_SkillTag;

        /// 技能效果列表（示意）：对游戏世界的影响，简单起见只用 Tag 表示。
        ME_PROPERTY()
        std::vector<minEngine::GameplayTag> m_Effects;
    };

    /// @brief 游戏中包含的所有技能的列表（内容目录，与某棵树解耦）。
    /// @note 正式项目中通常作为 DataAsset；本示例用反射类型表达同样契约。
    ME_CLASS()
    class SkillCatalog : public minEngine::DataAsset
    {
        ME_GENERATED_BODY()

    public:
        const std::vector<Skill>& GetSkills() const { return m_Skills; }
        void SetSkills(std::vector<Skill> skills) { m_Skills = std::move(skills); }
        void AddSkill(Skill skill) { m_Skills.push_back(std::move(skill)); }

        const Skill* FindSkill(const minEngine::GameplayTag& skillTag) const;

    private:
        ME_PROPERTY()
        std::vector<Skill> m_Skills;
    };

    /// @brief 学习对前置技能的要求：All=全部已学；Any=任一已学。
    /// @note 提到命名空间层，避免与 DefNode 成员同名遮蔽类型。
    ME_ENUM()
    enum class SkillPrerequisitePolicy
    {
        All,
        Any,
    };

    /// @brief 节点在技能树中的可见性策略（与能否学习独立计算）。
    ME_ENUM()
    enum class SkillRevealPolicy
    {
        Always,
        WhenAllPrerequisitesMet,
        WhenAnyPrerequisitesMet,
    };

    /// @brief 技能树定义节点：依赖、可见性、互斥与学习条件（费用真源在此，不在 Skill）。
    ME_STRUCT()
    struct SkillTreeDefNode
    {
        ME_GENERATED_BODY()

        /// 技能标签，用于唯一标识一个技能（相对本树）。
        ME_PROPERTY()
        minEngine::GameplayTag SkillTag;

        /// 独占组：同组技能互斥，只能择一学习。
        /// @note Tag 名为空或为 SkillTree.ExclusiveGroup.None 时表示不属于任何独占组。
        ME_PROPERTY()
        minEngine::GameplayTag ExclusiveGroupTag;

        ME_PROPERTY()
        std::vector<minEngine::GameplayTag> Prerequisites;

        ME_PROPERTY()
        SkillPrerequisitePolicy PrerequisitePolicy = SkillPrerequisitePolicy::All;

        ME_PROPERTY()
        SkillRevealPolicy RevealPolicy = SkillRevealPolicy::Always;

        /// 学习此技能需要满足的等级要求。
        ME_PROPERTY()
        int LevelRequirement = 0;

        /// 学习此技能需要消耗的技能点（费用真源）。
        ME_PROPERTY()
        int SkillPointCost = 0;
    };

    /// @brief 技能树定义：只定义节点依赖与学习条件，不承载技能内容列表与已学状态。
    /// @note 虽俗称「树」，许多技能树实际是有向无环图（DAG）。正式项目中可作为 DataAsset。
    ME_CLASS()
    class SkillTreeDef : public minEngine::DataAsset
    {
        ME_GENERATED_BODY()

    public:
        const std::string& GetName() const { return m_Name; }
        void SetName(std::string name) { m_Name = std::move(name); }

        const std::string& GetTreeTag() const { return m_TreeTag; }
        void SetTreeTag(std::string treeTag) { m_TreeTag = std::move(treeTag); }

        const std::string& GetDescription() const { return m_Description; }
        void SetDescription(std::string description) { m_Description = std::move(description); }

        const std::vector<SkillTreeDefNode>& GetNodes() const { return m_Nodes; }
        void SetNodes(std::vector<SkillTreeDefNode> nodes) { m_Nodes = std::move(nodes); }
        void AddNode(SkillTreeDefNode node) { m_Nodes.push_back(std::move(node)); }

        const SkillTreeDefNode* FindNode(const minEngine::GameplayTag& skillTag) const;

    private:
        ME_PROPERTY()
        std::string m_Name;

        ME_PROPERTY()
        std::string m_TreeTag;

        ME_PROPERTY()
        std::string m_Description;

        ME_PROPERTY()
        std::vector<SkillTreeDefNode> m_Nodes;
    };

    /// @brief Learn 时的外部上下文示意（等级、可用技能点等由宿主提供）。
    ME_STRUCT()
    struct SkillLearnContext
    {
        ME_GENERATED_BODY()

        ME_PROPERTY()
        int PlayerLevel = 1;

        ME_PROPERTY()
        int AvailableSkillPoints = 0;
    };

    ME_ENUM()
    enum class SkillLearnFailReason
    {
        None,
        UnknownSkill,
        AlreadyLearned,
        NotRevealed,
        PrerequisitesNotMet,
        LevelTooLow,
        NotEnoughSkillPoints,
        ExclusiveGroupConflict,
        InvalidDef,
    };

    ME_ENUM()
    enum class SkillTreeValidationErrorCode
    {
        DuplicateSkillTag,
        MissingPrerequisite,
        CycleDetected,
        EmptySkillTag,
    };

    ME_STRUCT()
    struct SkillTreeValidationError
    {
        ME_GENERATED_BODY()

        ME_PROPERTY()
        SkillTreeValidationErrorCode Code = SkillTreeValidationErrorCode::DuplicateSkillTag;

        ME_PROPERTY()
        std::string Message;

        ME_PROPERTY()
        minEngine::GameplayTag RelatedTag;
    };

    /// @brief 将校验（含 DAG）从 Instance 拆出，保持运行时类型信息简洁。
    /// @note 工具类不挂反射；校验逻辑与可序列化 Def 分离。
    class SkillTreeBuilder
    {
    public:
        /// @brief 校验节点唯一性、前驱存在性、以及依赖边构成 DAG。
        /// @return 全部通过为 true；错误写入 outErrors（可空）。
        static bool Validate(const SkillTreeDef& def, std::vector<SkillTreeValidationError>* outErrors);

        /// @brief 校验通过后返回可共享的只读 Def；失败返回 nullptr。
        static std::shared_ptr<const SkillTreeDef> Build(SkillTreeDef def,
                                                        std::vector<SkillTreeValidationError>* outErrors);
    };

    /// @brief 技能树运行时节点状态（供 UI 缓存；不在此存档指针）。
    /// @note 存档应只持久化已学 Tag 等；Revealed/CanLearn 由运行时按 Def + 已学集合刷新。
    /// @note 运行时缓存不挂反射，避免把瞬时 UI 状态写进资产管线。
    struct SkillTreeInstanceNode
    {
        const minEngine::GameplayTag& GetSkillTag() const { return m_SkillTag; }
        const SkillTreeDefNode* GetNodeDef() const { return m_NodeDef; }

        bool IsRevealed() const { return m_IsRevealed; }
        bool IsLearned() const { return m_IsLearned; }
        bool CanLearn() const { return m_CanLearn; }

    private:
        friend class SkillTreeInstance;

        minEngine::GameplayTag m_SkillTag;
        const SkillTreeDefNode* m_NodeDef = nullptr;

        /// 运行时状态：UI 常据此决定显示与交互，故缓存而非每次全量临时推算。
        /// 部分游戏在前驱未学时不向玩家展示节点信息。
        bool m_IsRevealed = false;
        bool m_IsLearned = false;
        bool m_CanLearn = false;
    };

    /// @brief 技能树运行时实例：持有只读 Def 与各节点动态状态。
    /// @note 运行时实例不挂反射；持久化走 CaptureLearnedSkills / ApplyLearnedSkills。
    class SkillTreeInstance
    {
    public:
        /// @brief 使用 Builder 校验通过后的 Def 初始化；会重建节点并刷新状态。
        bool Initialize(std::shared_ptr<const SkillTreeDef> def);

        const SkillTreeDef* GetDef() const { return m_Def.get(); }
        const std::vector<SkillTreeInstanceNode>& GetNodes() const { return m_Nodes; }
        const SkillTreeInstanceNode* FindNode(const minEngine::GameplayTag& skillTag) const;

        /// @brief 尝试学习；成功时扣除 context.AvailableSkillPoints 并刷新相关节点状态。
        bool TryLearn(const minEngine::GameplayTag& skillTag, SkillLearnContext& context,
                      SkillLearnFailReason* outFailReason = nullptr);

        /// @brief 仅已学 Tag 列表，适合作为存档载荷；读档后调用 ApplyLearnedSkills。
        std::vector<minEngine::GameplayTag> CaptureLearnedSkills() const;
        void ApplyLearnedSkills(const std::vector<minEngine::GameplayTag>& learnedSkills,
                                const SkillLearnContext* contextForCanLearn = nullptr);

        /// @param skillTag 变化的节点；@param node 刷新后的状态。
        minEngine::MulticastDelegate<const minEngine::GameplayTag&, const SkillTreeInstanceNode&> OnNodeStateChanged;

    private:
        void RebuildNodesFromDef();
        void RefreshAllNodeStates(const SkillLearnContext* context);
        void UpdateNodeState(SkillTreeInstanceNode& node, const SkillLearnContext* context);
        void NotifyNodeStateChanged(const SkillTreeInstanceNode& node);

        bool ArePrerequisitesMet(const SkillTreeDefNode& defNode) const;
        bool IsExclusiveGroupBlocked(const SkillTreeDefNode& defNode) const;
        static bool IsNoneExclusiveGroup(const minEngine::GameplayTag& tag);

        std::shared_ptr<const SkillTreeDef> m_Def;
        std::vector<SkillTreeInstanceNode> m_Nodes;
        std::unordered_map<std::string, size_t> m_TagToIndex;
    };
}

#include "SkillTree.gen.h"
