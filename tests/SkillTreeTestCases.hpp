#pragma once

#include "doctest.h"

#include "GameplayDataGraph/Examples/SkillTree.hpp"

#include "Runtime/Function/GameplayFramework/Tags/GameplayTag.h"

namespace
{
    minEngine::GameplayTag MakeTag(const char* name)
    {
        minEngine::GameplayTag tag;
        tag.SetTagName(name);
        return tag;
    }

    minGameplayFramework::SkillsTree::SkillTreeDefNode MakeNode(
        const char* tag,
        std::vector<const char*> prereqs = {},
        minGameplayFramework::SkillsTree::SkillPrerequisitePolicy prereqPolicy =
            minGameplayFramework::SkillsTree::SkillPrerequisitePolicy::All,
        minGameplayFramework::SkillsTree::SkillRevealPolicy reveal =
            minGameplayFramework::SkillsTree::SkillRevealPolicy::Always,
        int level = 1,
        int cost = 0,
        const char* exclusiveGroup = "")
    {
        using namespace minGameplayFramework::SkillsTree;
        SkillTreeDefNode node;
        node.SkillTag = MakeTag(tag);
        for (const char* prereq : prereqs)
        {
            node.Prerequisites.push_back(MakeTag(prereq));
        }
        node.PrerequisitePolicy = prereqPolicy;
        node.RevealPolicy = reveal;
        node.LevelRequirement = level;
        node.SkillPointCost = cost;
        if (exclusiveGroup != nullptr && exclusiveGroup[0] != '\0')
        {
            node.ExclusiveGroupTag = MakeTag(exclusiveGroup);
        }
        return node;
    }
}

TEST_CASE("skill tree builder rejects cycles and missing prerequisites")
{
    using namespace minGameplayFramework::SkillsTree;

    SkillTreeDef cyclic;
    cyclic.AddNode(MakeNode("Skill.A", {"Skill.B"}));
    cyclic.AddNode(MakeNode("Skill.B", {"Skill.A"}));

    std::vector<SkillTreeValidationError> errors;
    CHECK_FALSE(SkillTreeBuilder::Validate(cyclic, &errors));
    bool foundCycle = false;
    for (const SkillTreeValidationError& error : errors)
    {
        if (error.Code == SkillTreeValidationErrorCode::CycleDetected)
        {
            foundCycle = true;
        }
    }
    CHECK(foundCycle);

    SkillTreeDef missing;
    missing.AddNode(MakeNode("Skill.A", {"Skill.Missing"}));
    errors.clear();
    CHECK_FALSE(SkillTreeBuilder::Validate(missing, &errors));
}

TEST_CASE("skill tree learn respects all prerequisites cost and refreshes unlocks")
{
    using namespace minGameplayFramework::SkillsTree;

    SkillTreeDef def;
    def.AddNode(MakeNode("Skill.Basic", {}, SkillPrerequisitePolicy::All, SkillRevealPolicy::Always, 1, 0));
    def.AddNode(MakeNode("Skill.Power", {"Skill.Basic"}, SkillPrerequisitePolicy::All,
                         SkillRevealPolicy::WhenAnyPrerequisitesMet, 1, 1));
    def.AddNode(MakeNode("Skill.Sweep", {"Skill.Basic"}, SkillPrerequisitePolicy::All,
                         SkillRevealPolicy::WhenAnyPrerequisitesMet, 1, 1));
    def.AddNode(MakeNode("Skill.Ultimate", {"Skill.Power", "Skill.Sweep"}, SkillPrerequisitePolicy::All,
                         SkillRevealPolicy::WhenAllPrerequisitesMet, 1, 2));

    auto built = SkillTreeBuilder::Build(std::move(def), nullptr);
    REQUIRE(built != nullptr);

    SkillTreeInstance instance;
    REQUIRE(instance.Initialize(built));

    // SkillLearnContext：宿主传入当前等级与可用技能点；Learn 成功会就地扣点。
    SkillLearnContext context;
    context.PlayerLevel = 10;
    context.AvailableSkillPoints = 10;

    SkillLearnFailReason fail = SkillLearnFailReason::None;
    // WhenAllPrerequisitesMet：前置未齐时节点未揭示，优先返回 NotRevealed。
    CHECK_FALSE(instance.TryLearn(MakeTag("Skill.Ultimate"), context, &fail));
    CHECK(fail == SkillLearnFailReason::NotRevealed);

    REQUIRE(instance.TryLearn(MakeTag("Skill.Basic"), context, &fail));
    CHECK(instance.FindNode(MakeTag("Skill.Power"))->IsRevealed());
    CHECK(instance.FindNode(MakeTag("Skill.Power"))->CanLearn());

    REQUIRE(instance.TryLearn(MakeTag("Skill.Power"), context, &fail));
    REQUIRE(instance.TryLearn(MakeTag("Skill.Sweep"), context, &fail));
    CHECK(instance.FindNode(MakeTag("Skill.Ultimate"))->CanLearn());
    REQUIRE(instance.TryLearn(MakeTag("Skill.Ultimate"), context, &fail));
    CHECK(context.AvailableSkillPoints == 6); // 10 - 0 - 1 - 1 - 2
}

TEST_CASE("skill tree exclusive group blocks sibling learn")
{
    using namespace minGameplayFramework::SkillsTree;

    SkillTreeDef def;
    def.AddNode(MakeNode("Skill.Root", {}, SkillPrerequisitePolicy::All, SkillRevealPolicy::Always, 1, 0));
    def.AddNode(MakeNode("Skill.PathA", {"Skill.Root"}, SkillPrerequisitePolicy::All,
                         SkillRevealPolicy::Always, 1, 1, "SkillTree.ExclusiveGroup.Stance"));
    def.AddNode(MakeNode("Skill.PathB", {"Skill.Root"}, SkillPrerequisitePolicy::All,
                         SkillRevealPolicy::Always, 1, 1, "SkillTree.ExclusiveGroup.Stance"));

    auto built = SkillTreeBuilder::Build(std::move(def), nullptr);
    REQUIRE(built != nullptr);

    SkillTreeInstance instance;
    REQUIRE(instance.Initialize(built));

    SkillLearnContext context;
    context.PlayerLevel = 1;
    context.AvailableSkillPoints = 5;

    REQUIRE(instance.TryLearn(MakeTag("Skill.Root"), context, nullptr));
    REQUIRE(instance.TryLearn(MakeTag("Skill.PathA"), context, nullptr));

    SkillLearnFailReason fail = SkillLearnFailReason::None;
    CHECK_FALSE(instance.TryLearn(MakeTag("Skill.PathB"), context, &fail));
    CHECK(fail == SkillLearnFailReason::ExclusiveGroupConflict);
}

TEST_CASE("skill tree capture and apply learned skills")
{
    using namespace minGameplayFramework::SkillsTree;

    SkillTreeDef def;
    def.AddNode(MakeNode("Skill.Basic"));
    def.AddNode(MakeNode("Skill.Power", {"Skill.Basic"}, SkillPrerequisitePolicy::All,
                         SkillRevealPolicy::Always, 1, 0));

    auto built = SkillTreeBuilder::Build(std::move(def), nullptr);
    SkillTreeInstance instance;
    REQUIRE(instance.Initialize(built));

    SkillLearnContext context;
    context.PlayerLevel = 1;
    context.AvailableSkillPoints = 0;
    REQUIRE(instance.TryLearn(MakeTag("Skill.Basic"), context, nullptr));
    REQUIRE(instance.TryLearn(MakeTag("Skill.Power"), context, nullptr));

    const std::vector<minEngine::GameplayTag> saved = instance.CaptureLearnedSkills();

    SkillTreeInstance loaded;
    REQUIRE(loaded.Initialize(built));
    loaded.ApplyLearnedSkills(saved, &context);
    CHECK(loaded.FindNode(MakeTag("Skill.Power"))->IsLearned());
}
