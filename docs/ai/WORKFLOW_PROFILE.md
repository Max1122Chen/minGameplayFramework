# Workflow Profile（工作流画像）

Last updated: 2026-10-26
Status: `configured`
<!-- Allowed status: unconfigured | configured | deferred | default-applied -->

本文件是本仓协作姿态的唯一运行时真相。Agent 在 bootstrap 阶段必须读取它。
当前状态为 `configured`：新会话不再询问配置，直接按下面的 Effective behavior 工作；
只有用户明确说 `reconfigure workflow` 时才重新走配置流程。

## Current selection

| Field | Value |
|-------|-------|
| `preset_id` | `serious-engineering` |
| `repo_posture` | `serious-engineering` |
| `autonomy` | `propose-then-act` |
| `agent_role` | `partner` |
| `docs_language` | `zh` |
| `docs_topology` | `single-track` |
| `verification_bar` | `docs-only` |
| `implementation_discipline` | `on` |
| `product_truth_path` | `n/a` |

## Effective behavior（派生行为）

- Challenge skipped workflow：**high** — 对"看起来做完了"的空壳产物、缺失的验收、只停在口号的设计原则，必须明确质疑，不能顺着往下做。
- Pre-flight（大型 Feature / 重构）：**required** — 先写一句复杂度级别与理由（L0–L3），再决定仪式；L2+ 先设计后编码。
- Pause for approval：**仅在 L2+ 设计与范围扩张时暂停** — L0/L1 与既定范围内的实施不停顿、不逐条请示。
- Default explanation depth：**normal** — 给结论、给理由、给证据；不做教学式长篇铺垫，也不省略关键取舍。
- New docs / progress language：**zh** — 项目自有文档用中文；从模板继承的治理文档、`.cursor` 规则、技能保留英文原文。
- Engineering DoD bar：当前 **docs-only** — 验收 = 文档自检（占位符扫描、命名与 ID 合规、交叉引用指向真实路径），无构建、无测试。
  切换条件：**首个代码 Feature 落地时**，本行与上表 `verification_bar` 同时升为 `tests-required`，
  在 `PROJECT_CONTEXT.md` 登记构建与测试命令，并在 `PROGRESS_LOG.md` 记录这次切换。

## Non-negotiable constraints（profile 不可关闭）

- 准备提交 ≠ 执行提交：可以起草 diff 与提交信息；`git commit`、push、改写历史只在用户明确指示后执行。
- 只从可信来源规划：`ACTIVE_WORK.md`、`FEATURE_REGISTRY.md` 的 In Progress/Planned、未关闭的 `TECH_DEBT.md`、近期 `PROGRESS_LOG.md`、代码与验证输出。
- `Draft` 设计不授权大规模编码；设计评审"未就绪"同样不授权。
- L3 架构安全检查（所有权 / 契约 / 迁移 / 失败路径）不可被 preset 跳过。
- 源码注释、日志、测试名不得出现 Feature / Slice / Bug / ADR / TechDebt 编号，也不得出现设计文档章节号（如 §3.4、H8）。
- 分支名是主题粒度：`feat/<topic>[/<subtopic>]`，不得使用登记 ID。
- L2+ 必须有实质 DoD：内容是成立的，而不是只把模板壳填满。

## Reconfigure

用户口令：`reconfigure workflow`
Agent 动作：重新走一遍预设 / 维度询问，改写本文件，并一句话总结"改了什么、影响哪些行为"。
