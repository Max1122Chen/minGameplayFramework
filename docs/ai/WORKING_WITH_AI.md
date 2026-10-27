# 与 AI 协作

Last updated: 2026-10-26

## 本仓特点（先记住这三条）

- **探索期代码先行（翻转常规流程）**：作者用伪代码 / 草稿代码 / 口头需求表达设计；agent 读代码与意图，修订成正式代码供审批。可另落**薄设计文档**作执行参照供作者 review，但详细契约与「为什么」以**代码注释**为主（代码即文档）。归档的 CORE design v1（`docs/deprecated/`）不得作实现依据。
- **注释：中文先行 → 审批后再补英文**：正式路径注释用 Doxygen；写清 why、解决什么问题、迁移注意。先中文版供作者审；通过后再补英文，最终中英双语各一份。禁止 Feature / Slice / Bug / ADR 编号与设计章节号。
- **参考仓只读**：minEngine、minCraft、min-agent-workflows 只借不改。

## 探索期协作节奏（code → review → docs）

1. 作者写伪代码 / 不完整头文件 / 随手注释，或口述需求。
2. Agent 回放意图、指出歧义；需要时先落**薄设计文档**（执行参照，非空壳长文）供 review。
3. 按审批结论写成可编译正式代码；注释先中文 Doxygen（why / 问题 / 迁移），作者通过后再补英文。
4. 行为稳定后：必要时把设计文档升为当前契约并登记 Feature。空壳设计仍拒绝。

## 会话开始（bootstrap）

~~~text
继续本仓。按 docs/ai/BOOTSTRAP_DIGEST.md 的顺序恢复上下文：
WORKFLOW_PROFILE.md、PROJECT_CONTEXT.md、PROGRESS_LOG.md 最近条目、ACTIVE_WORK.md、FEATURE_REGISTRY.md、TECH_DEBT.md。
用三句话总结协作姿态与当前状态，然后给出下一步建议，不要直接开始写实现。
~~~

## 重新配置工作流（reconfigure workflow）

~~~text
reconfigure workflow
~~~

期望动作：重新走预设 / 维度询问，改写 `WORKFLOW_PROFILE.md`，并说明改了什么、影响哪些行为。

## 设计文档（薄执行参照 + 后期沉淀）

本仓设计文档要求（相对模板可裁剪，但不可空壳）：

- **薄**：问题、范围、非目标、关键契约、示例清单、失败路径；数据结构点到为止（细节在代码）。
- **代码即文档**：类型职责、组合理由、迁移注意写在源码注释里，设计文档不重复粘贴伪代码。
- **可执行**：能当正式化 checklist 用；作者 review 通过前 status 保持 `Draft`。
- 模板仍可用：`docs/ai/templates/design-spec.template.md`（只填相关节）。

~~~text
先写一句复杂度级别与理由（L0–L3）。以作者伪代码 / 口头 / 已有草稿为输入，
落薄设计文档供 review；不要臆造作者尚未表达的模块；不要把归档 design v1 当约束。
~~~

## 设计评审（design-review）

~~~text
按 docs/ai/templates/DOC_GOVERNANCE.md 的就绪门槛评审这份设计：
需求与方案是否分离、契约是否可验证、边界与失败路径是否交代、是否存在空壳 TBD。
结论只给：Ready / Ready with deferred items / Not ready，并列出必需修改项。
探索期允许无设计文档的正式编码；一旦写出设计文档，未通过评审的草稿不得冒充当前契约。
~~~

## 交接（handoff）

~~~text
交接：按 docs/ai/templates/session-note.template.md 写会话记录，
向 docs/ai/PROGRESS_LOG.md 追加一条，未完成的 Slice 标为 Blocked/Deferred 并写清原因的解除条件。
~~~

## 登记 Feature（register-feature）

~~~text
登记 Feature：在 docs/ai/FEATURE_REGISTRY.md 追加一行，领域码只用已启用的
（CORE / GPCONTENT / ITEM / INVENTORY 等），ID 形如 GPCONTENT-F01。
需要新领域码时先问我，再同时更新 FEATURE_REGISTRY.md 与 BOOTSTRAP_DIGEST.md。
~~~

## 协作习惯

- 实质性新工作：探索期以作者代码 / 口头为准；行为稳定并要对外宣称契约时再登记 Feature ID 与设计文档。
- 请求有歧义：先用约 200 字回放意图，再问一个关键问题（用户可跳过）。
- 区分"要解决的问题"与"提议的方案"；先看作者草稿与可信规划源，再动手。
- 对空壳产物保持挑战：关键部分只有 TBD、验收不可验证、原则只停在口号，都不算完成。
- 架构或范围决策：稳定后写入设计文档，必要时补 ADR（`templates/adr.template.md`）。
- 跨模块缺陷：先按 `templates/bug-record.template.md` 记录，再做批量顺手修复。
- 提交：先起草（diff + 提交信息），用户明确同意后才执行 `git commit`。
- 严格度、自主度、语言、验证门槛一律以 `WORKFLOW_PROFILE.md` 的 Effective behavior 为准。
