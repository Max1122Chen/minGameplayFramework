# Bootstrap Digest（会话快速恢复）

Last updated: 2026-10-27
用途：新会话在 2 分钟内恢复协作上下文。

## 新会话读取顺序

1. `WORKFLOW_PROFILE.md` — 协作姿态（当前为 `configured`，不再询问配置）
2. `PROJECT_CONTEXT.md`
3. `PROGRESS_LOG.md` — 只读最近若干条，不逐条回溯历史
4. `ACTIVE_WORK.md`
5. `FEATURE_REGISTRY.md` — 关注 In Progress / Planned
6. `TECH_DEBT.md` — Open
7. 任务相关的设计文档（由 ACTIVE_WORK 链接，或用户点名）；探索期同时看作者正在写的 `Source/` 草稿

## Workflow profile 门禁

- 预设目录：`templates/WORKFLOW_PRESETS.md`
- `unconfigured`：一次性询问「现在配置 / 采用 serious-engineering 默认 / 稍后」，之后不再问。
- `deferred`：不再询问，除非用户说 `reconfigure workflow`。
- `configured`（本仓当前状态）/ `default-applied`：直接按 `WORKFLOW_PROFILE.md` 的 Effective behavior 执行。

## 非协商协作规则

- 只从可信来源规划；不要从旧 roadmap、模板示例、历史设计里的勾选清单、`docs/deprecated/` 归档件推断待办。
- 新的实质性工作：先写一句复杂度级别（L0–L3）+ 理由。本仓探索期默认**代码 / 伪代码先行**（见 `WORKING_WITH_AI.md`）；稳定后再登记 Feature 并提炼设计文档。
- 已写出的 `Draft` 设计或评审「未就绪」文档不得冒充当前契约；空壳设计仍拒绝。
- L3 架构安全检查（所有权 / 契约 / 迁移 / 失败路径）不可被 profile 跳过——可用代码草稿 + 意图回放承担，不强制先出长文。
- 一个有意义的批次结束：先过 Process + Quality DoD，再提议「准备提交」，然后才进入不相关的下一件事。
- 准备提交只包含起草与评审；`git commit` / push 只在用户明确指示后执行。
- 源码注释：Doxygen；正式化先中文（why / 问题 / 迁移），作者审过再补英文，最终双语；注释与测试名不得出现 Feature / Slice / Bug / ADR / TechDebt 编号或设计章节号；分支名用 `feat/<topic>[/<subtopic>]`，不用 ID。
- 设计文档宜薄（执行参照）；详细契约与设计思想以代码注释为准。
- Profile 可以调整严格度、自主度、角色、语言、文档拓扑、验证基线；不能关闭以上任何一条。

## 工程设计与评审

- 技能：`.opencode/skills/engineering-design/`、`.opencode/skills/design-review/`
- 流程与就绪门槛：`templates/DOC_GOVERNANCE.md`
- 设计原则透镜：`templates/DESIGN_PRINCIPLES.md`
- 本仓默认路径：作者草稿代码 → agent 正式化与审批 → Doxygen → 设计文档提炼。
- 若某次工作明确选择「先写设计再编码」，仍走 Design → Design Review → Implementation Plan；L3 追加 readiness 门禁。

## ID 方案

- Feature：`<DOMAIN>-F<nn>`
- Slice：`<FeatureID>-S<nn>`
- Bug：`BUG-<DOMAIN>-<nnn>`
- Tech Debt：`TD-<nnn>`
- ADR：`ADR-<yyyyMMdd>-<nn>`
- 长文档（全大写）：`docs/ai/<DOMAIN>/<FEATURE_ID>_<SLUG>_<DOC_TYPE>.md`，`<DOC_TYPE>` ∈ `DESIGN` | `IMPLEMENTATION` | `ROADMAP` | `REFACTOR_PLAN`

## 领域码

- 已启用：`CORE`（预留）、`GPCONTENT`、`ITEM`、`INVENTORY`、`SKILL`。
- 已落地：`GPCONTENT-F01` / `ITEM-F01` / `INVENTORY-F01`（Done）。
- 活跃：`SKILL-F01`（SkillTree 示例，Review）。
- 其他子系统码在首个真实需求出现时按 `FEATURE_REGISTRY.md` 追加；不得预置、不建空桶。

## 验证基线

- 当前 `tests-required`：文档自检 + 可复现构建 / 测试。
- 构建：`cmake --build build --target MGF_Tests`（配置见仓库根 `CMakeLists.txt`，`ENGINE_ROOT` 指向 minEngine）。
- 测试：`bin/MGF_Tests.exe`（或 CTest 目标 `MGF_Tests`）。

## 交接触发条件

当用户表示交接或切换会话时：
- 按 `templates/session-note.template.md` 写一份会话记录；
- 向 `PROGRESS_LOG.md` 追加一条；
- 未完成的 Slice 明确标为 Blocked / Deferred，写清原因与解除条件。
