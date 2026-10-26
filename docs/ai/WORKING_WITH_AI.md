# 与 AI 协作

Last updated: 2026-10-26

## 本仓特点（先记住这三条）

- **设计优先**：先写设计（分层、契约、不变量、失败路径），评审通过再谈实现；`Draft` 不授权大规模编码。
- **注释里不写 ID**：源码注释英文、Doxygen 风格，禁止出现 Feature / Slice / Bug / ADR 编号与设计章节号。
- **参考仓只读**：minEngine、minCraft、min-agent-workflows 只借不改。

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

## 设计（design）

~~~text
先写一句复杂度级别与理由（L0–L3）。若为 L2+，按 docs/ai/templates/design-spec.template.md 出设计文档：
需求与方案分开陈述，写清分层、契约、不变量、失败路径与不可执行的 TBD。
不要臆造尚未设计的模块——框架分层与模块边界目前尚未确定。
~~~

## 设计评审（design-review）

~~~text
按 docs/ai/templates/DOC_GOVERNANCE.md 的就绪门槛评审这份设计：
需求与方案是否分离、契约是否可验证、边界与失败路径是否交代、是否存在空壳 TBD。
结论只给：Ready / Ready with deferred items / Not ready，并列出必需修改项。
评审未通过前不要进入实施规划。
~~~

## 交接（handoff）

~~~text
交接：按 docs/ai/templates/session-note.template.md 写会话记录，
向 docs/ai/PROGRESS_LOG.md 追加一条，未完成的 Slice 标为 Blocked/Deferred 并写清原因的解除条件。
~~~

## 登记 Feature（register-feature）

~~~text
登记 Feature：在 docs/ai/FEATURE_REGISTRY.md 追加一行，领域码只用已启用的（当前仅 CORE），
ID 形如 CORE-F01。需要新领域码时先问我，再同时更新 FEATURE_REGISTRY.md 与 BOOTSTRAP_DIGEST.md。
~~~

## 协作习惯

- 实质性新工作：先登记 Feature ID，再做大规模改动。
- 请求有歧义：先用约 200 字回放意图，再问一个关键问题（用户可跳过）。
- 区分"要解决的问题"与"提议的方案"；先看现有设计再动手。
- 对空壳产物保持挑战：关键部分只有 TBD、验收不可验证、原则只停在口号，都不算完成。
- 架构或范围决策：更新设计文档，必要时补一份 ADR（`templates/adr.template.md`）。
- 跨模块缺陷：先按 `templates/bug-record.template.md` 记录，再做批量顺手修复。
- 提交：先起草（diff + 提交信息），用户明确同意后才执行 `git commit`。
- 严格度、自主度、语言、验证门槛一律以 `WORKFLOW_PROFILE.md` 的 Effective behavior 为准。
