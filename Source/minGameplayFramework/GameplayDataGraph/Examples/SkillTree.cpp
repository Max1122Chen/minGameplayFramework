#include "GameplayDataGraph/Examples/SkillTree.hpp"

#include <queue>
#include <unordered_set>

namespace minGameplayFramework::SkillsTree
{
    namespace
    {
        const char* kNoneExclusiveGroupName = "SkillTree.ExclusiveGroup.None";

        std::string TagKey(const minEngine::GameplayTag& tag)
        {
            return tag.GetName();
        }
    }

    const Skill* SkillCatalog::FindSkill(const minEngine::GameplayTag& skillTag) const
    {
        for (const Skill& skill : m_Skills)
        {
            if (skill.GetSkillTag() == skillTag)
            {
                return &skill;
            }
        }
        return nullptr;
    }

    const SkillTreeDefNode* SkillTreeDef::FindNode(const minEngine::GameplayTag& skillTag) const
    {
        for (const SkillTreeDefNode& node : m_Nodes)
        {
            if (node.SkillTag == skillTag)
            {
                return &node;
            }
        }
        return nullptr;
    }

    bool SkillTreeBuilder::Validate(const SkillTreeDef& def, std::vector<SkillTreeValidationError>* outErrors)
    {
        if (outErrors != nullptr)
        {
            outErrors->clear();
        }

        bool ok = true;
        auto pushError = [&](SkillTreeValidationErrorCode code, std::string message,
                             minEngine::GameplayTag related = {}) {
            ok = false;
            if (outErrors == nullptr)
            {
                return;
            }
            SkillTreeValidationError error;
            error.Code = code;
            error.Message = std::move(message);
            error.RelatedTag = std::move(related);
            outErrors->push_back(std::move(error));
        };

        std::unordered_map<std::string, size_t> tagToIndex;
        const std::vector<SkillTreeDefNode>& nodes = def.GetNodes();
        tagToIndex.reserve(nodes.size());

        for (size_t i = 0; i < nodes.size(); ++i)
        {
            const SkillTreeDefNode& node = nodes[i];
            // 身份以 TagName 为准；示例/单测可不注册 GameplayTagManager。
            if (node.SkillTag.GetName().empty())
            {
                pushError(SkillTreeValidationErrorCode::EmptySkillTag, "SkillTag is empty", node.SkillTag);
                continue;
            }

            const std::string key = TagKey(node.SkillTag);
            if (tagToIndex.find(key) != tagToIndex.end())
            {
                pushError(SkillTreeValidationErrorCode::DuplicateSkillTag,
                          "Duplicate SkillTag in tree def", node.SkillTag);
            }
            else
            {
                tagToIndex.emplace(key, i);
            }
        }

        std::vector<std::vector<size_t>> adjacency(nodes.size());
        std::vector<int> indegree(nodes.size(), 0);

        for (size_t i = 0; i < nodes.size(); ++i)
        {
            const SkillTreeDefNode& node = nodes[i];
            for (const minEngine::GameplayTag& prereq : node.Prerequisites)
            {
                const auto it = tagToIndex.find(TagKey(prereq));
                if (it == tagToIndex.end())
                {
                    pushError(SkillTreeValidationErrorCode::MissingPrerequisite,
                              "Prerequisite tag is not a node in this tree", prereq);
                    continue;
                }
                // Edge: prerequisite -> node（学习顺序上前驱在前）。
                adjacency[it->second].push_back(i);
                ++indegree[i];
            }
        }

        std::queue<size_t> ready;
        for (size_t i = 0; i < nodes.size(); ++i)
        {
            if (indegree[i] == 0)
            {
                ready.push(i);
            }
        }

        size_t visited = 0;
        while (!ready.empty())
        {
            const size_t index = ready.front();
            ready.pop();
            ++visited;
            for (size_t next : adjacency[index])
            {
                if (--indegree[next] == 0)
                {
                    ready.push(next);
                }
            }
        }

        if (visited != nodes.size())
        {
            pushError(SkillTreeValidationErrorCode::CycleDetected,
                      "Skill tree prerequisites contain a cycle (not a DAG)", {});
        }

        return ok;
    }

    std::shared_ptr<const SkillTreeDef> SkillTreeBuilder::Build(SkillTreeDef def,
                                                               std::vector<SkillTreeValidationError>* outErrors)
    {
        if (!Validate(def, outErrors))
        {
            return nullptr;
        }
        return std::make_shared<const SkillTreeDef>(std::move(def));
    }

    bool SkillTreeInstance::IsNoneExclusiveGroup(const minEngine::GameplayTag& tag)
    {
        return tag.GetName().empty() || tag.GetName() == kNoneExclusiveGroupName;
    }

    bool SkillTreeInstance::Initialize(std::shared_ptr<const SkillTreeDef> def)
    {
        if (def == nullptr)
        {
            return false;
        }
        std::vector<SkillTreeValidationError> errors;
        if (!SkillTreeBuilder::Validate(*def, &errors))
        {
            return false;
        }

        m_Def = std::move(def);
        RebuildNodesFromDef();
        RefreshAllNodeStates(nullptr);
        return true;
    }

    void SkillTreeInstance::RebuildNodesFromDef()
    {
        m_Nodes.clear();
        m_TagToIndex.clear();
        if (m_Def == nullptr)
        {
            return;
        }

        const std::vector<SkillTreeDefNode>& defNodes = m_Def->GetNodes();
        m_Nodes.reserve(defNodes.size());
        for (size_t i = 0; i < defNodes.size(); ++i)
        {
            SkillTreeInstanceNode node;
            node.m_SkillTag = defNodes[i].SkillTag;
            node.m_NodeDef = &defNodes[i];
            node.m_IsRevealed = false;
            node.m_IsLearned = false;
            node.m_CanLearn = false;
            m_TagToIndex.emplace(TagKey(node.m_SkillTag), i);
            m_Nodes.push_back(std::move(node));
        }
    }

    const SkillTreeInstanceNode* SkillTreeInstance::FindNode(const minEngine::GameplayTag& skillTag) const
    {
        const auto it = m_TagToIndex.find(TagKey(skillTag));
        if (it == m_TagToIndex.end())
        {
            return nullptr;
        }
        return &m_Nodes[it->second];
    }

    bool SkillTreeInstance::ArePrerequisitesMet(const SkillTreeDefNode& defNode) const
    {
        if (defNode.Prerequisites.empty())
        {
            return true;
        }

        size_t learnedCount = 0;
        for (const minEngine::GameplayTag& prereq : defNode.Prerequisites)
        {
            const SkillTreeInstanceNode* node = FindNode(prereq);
            if (node != nullptr && node->IsLearned())
            {
                ++learnedCount;
            }
        }

        if (defNode.PrerequisitePolicy == SkillPrerequisitePolicy::Any)
        {
            return learnedCount > 0;
        }
        return learnedCount == defNode.Prerequisites.size();
    }

    bool SkillTreeInstance::IsExclusiveGroupBlocked(const SkillTreeDefNode& defNode) const
    {
        if (IsNoneExclusiveGroup(defNode.ExclusiveGroupTag))
        {
            return false;
        }

        for (const SkillTreeInstanceNode& other : m_Nodes)
        {
            if (other.m_NodeDef == nullptr || !other.IsLearned())
            {
                continue;
            }
            if (other.GetSkillTag() == defNode.SkillTag)
            {
                continue;
            }
            if (other.m_NodeDef->ExclusiveGroupTag == defNode.ExclusiveGroupTag)
            {
                return true;
            }
        }
        return false;
    }

    void SkillTreeInstance::UpdateNodeState(SkillTreeInstanceNode& node, const SkillLearnContext* context)
    {
        if (node.m_NodeDef == nullptr)
        {
            return;
        }

        const SkillTreeDefNode& defNode = *node.m_NodeDef;
        const bool prereqMet = ArePrerequisitesMet(defNode);

        size_t learnedPrereqCount = 0;
        for (const minEngine::GameplayTag& prereq : defNode.Prerequisites)
        {
            const SkillTreeInstanceNode* prereqNode = FindNode(prereq);
            if (prereqNode != nullptr && prereqNode->IsLearned())
            {
                ++learnedPrereqCount;
            }
        }

        // Reveal 与前置策略独立：可见性只按 RevealPolicy 计算。
        bool revealed = false;
        switch (defNode.RevealPolicy)
        {
        case SkillRevealPolicy::Always:
            revealed = true;
            break;
        case SkillRevealPolicy::WhenAllPrerequisitesMet:
            revealed = defNode.Prerequisites.empty() ||
                       learnedPrereqCount == defNode.Prerequisites.size();
            break;
        case SkillRevealPolicy::WhenAnyPrerequisitesMet:
            revealed = defNode.Prerequisites.empty() || learnedPrereqCount > 0;
            break;
        }

        const bool canLearn = !node.m_IsLearned && revealed && prereqMet &&
                              !IsExclusiveGroupBlocked(defNode) &&
                              (context == nullptr ||
                               (context->PlayerLevel >= defNode.LevelRequirement &&
                                context->AvailableSkillPoints >= defNode.SkillPointCost));

        const bool revealedChanged = node.m_IsRevealed != revealed;
        const bool canLearnChanged = node.m_CanLearn != canLearn;
        node.m_IsRevealed = revealed;
        node.m_CanLearn = canLearn;

        if (revealedChanged || canLearnChanged)
        {
            NotifyNodeStateChanged(node);
        }
    }

    void SkillTreeInstance::RefreshAllNodeStates(const SkillLearnContext* context)
    {
        for (SkillTreeInstanceNode& node : m_Nodes)
        {
            UpdateNodeState(node, context);
        }
    }

    void SkillTreeInstance::NotifyNodeStateChanged(const SkillTreeInstanceNode& node)
    {
        OnNodeStateChanged.Broadcast(node.GetSkillTag(), node);
    }

    bool SkillTreeInstance::TryLearn(const minEngine::GameplayTag& skillTag, SkillLearnContext& context,
                                     SkillLearnFailReason* outFailReason)
    {
        auto setFail = [&](SkillLearnFailReason reason) {
            if (outFailReason != nullptr)
            {
                *outFailReason = reason;
            }
        };

        if (m_Def == nullptr)
        {
            setFail(SkillLearnFailReason::InvalidDef);
            return false;
        }

        const auto it = m_TagToIndex.find(TagKey(skillTag));
        if (it == m_TagToIndex.end())
        {
            setFail(SkillLearnFailReason::UnknownSkill);
            return false;
        }

        SkillTreeInstanceNode& node = m_Nodes[it->second];
        RefreshAllNodeStates(&context);

        if (node.IsLearned())
        {
            setFail(SkillLearnFailReason::AlreadyLearned);
            return false;
        }
        if (!node.IsRevealed())
        {
            setFail(SkillLearnFailReason::NotRevealed);
            return false;
        }
        if (node.m_NodeDef == nullptr)
        {
            setFail(SkillLearnFailReason::InvalidDef);
            return false;
        }

        const SkillTreeDefNode& defNode = *node.m_NodeDef;
        if (!ArePrerequisitesMet(defNode))
        {
            setFail(SkillLearnFailReason::PrerequisitesNotMet);
            return false;
        }
        if (context.PlayerLevel < defNode.LevelRequirement)
        {
            setFail(SkillLearnFailReason::LevelTooLow);
            return false;
        }
        if (context.AvailableSkillPoints < defNode.SkillPointCost)
        {
            setFail(SkillLearnFailReason::NotEnoughSkillPoints);
            return false;
        }
        if (IsExclusiveGroupBlocked(defNode))
        {
            setFail(SkillLearnFailReason::ExclusiveGroupConflict);
            return false;
        }

        node.m_IsLearned = true;
        context.AvailableSkillPoints -= defNode.SkillPointCost;
        NotifyNodeStateChanged(node);
        RefreshAllNodeStates(&context);

        setFail(SkillLearnFailReason::None);
        return true;
    }

    std::vector<minEngine::GameplayTag> SkillTreeInstance::CaptureLearnedSkills() const
    {
        std::vector<minEngine::GameplayTag> learned;
        for (const SkillTreeInstanceNode& node : m_Nodes)
        {
            if (node.IsLearned())
            {
                learned.push_back(node.GetSkillTag());
            }
        }
        return learned;
    }

    void SkillTreeInstance::ApplyLearnedSkills(const std::vector<minEngine::GameplayTag>& learnedSkills,
                                               const SkillLearnContext* contextForCanLearn)
    {
        for (SkillTreeInstanceNode& node : m_Nodes)
        {
            node.m_IsLearned = false;
        }
        for (const minEngine::GameplayTag& tag : learnedSkills)
        {
            const auto it = m_TagToIndex.find(TagKey(tag));
            if (it != m_TagToIndex.end())
            {
                m_Nodes[it->second].m_IsLearned = true;
            }
        }
        RefreshAllNodeStates(contextForCanLearn);
        for (const SkillTreeInstanceNode& node : m_Nodes)
        {
            NotifyNodeStateChanged(node);
        }
    }
}
