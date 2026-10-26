# Bootstrap Digest（会话快速恢复）

Last updated: 2026-10-26
用途：新会话在 2 分钟内恢复协作上下文。

## 新会话读取顺序

1. `WORKFLOW_PROFILE.md` — 协作姿态（当前为 `configured`，不再询问配置）
2. `PROJECT_CONTEXT.md`
3. `PROGRESS_LOG.md` — 只读最近若干条，不逐条回溯历史
4. `ACTIVE_WORK.md`
5. `FEATURE_REGISTRY.md` — 关注 In Progress / Planned
6. `TECH_DEBT.md` — Open
7. 任务相关的设计文档（由 ACTIVE_WORK 链接，或用户点名）

## Workflow profile 门禁

- 预设目录：`templates/WORKFLOW_PRESETS.md`
- `unconfigured`：一次性询问「现在配置 / 采用 serious-engineering 默认 / 稍后」，之后不再问。
- `deferred`：不再询问，除非用户说 `reconfigure workflow`。
- `configured`（本仓当前状态）/ `default-applied`：直接按 `WORKFLOW_PROFILE.md` 的 Effective behavior 执行。

## 非协商协作规则

- 只从可信来源规划；不要从旧 roadmap、模板示例、历史设计里的勾选清单推断待办。
- 新的实质性工作：先写一句复杂度级别（L0–L3）+ 理由；登记 ID；L2+ 先设计再大规模编码。
- `Draft` 设计或设计评审「未就绪」都不授权大规模编码。
- L3 架构安全检查不可被 profile 跳过。
- 一个有意义的批次结束：先过 Process + Quality DoD，再提议「准备提交」，然后才进入不相关的下一件事。
- 准备提交只包含起草与评审；`git commit` / push 只在用户明确指示后执行。
- 源码注释、日志字符串、测试用例名不得出现 Feature / Slice / Bug / ADR / TechDebt 编号或设计文档章节号；分支名用 `feat/<topic>[/<subtopic>]`，不用 ID。
- Profile 可以调整严格度、自主度、角色、语言、文档拓扑、验证基线；不能关闭以上任何一条。

## 工程设计与评审

- 技能：`.opencode/skills/engineering-design/`、`.opencode/skills/design-review/`
- 流程与就绪门槛：`templates/DOC_GOVERNANCE.md`
- 设计原则透镜：`templates/DESIGN_PRINCIPLES.md`
- L0 可跳过设计；L2+ 走 Design → Design Review → Implementation Plan；L3 追加 readiness 门禁。

## ID 方案

- Feature：`<DOMAIN>-F<nn>`
- Slice：`<FeatureID>-S<nn>`
- Bug：`BUG-<DOMAIN>-<nnn>`
- Tech Debt：`TD-<nnn>`
- ADR：`ADR-<yyyyMMdd>-<nn>`
- 长文档（全大写）：`docs/ai/<DOMAIN>/<FEATURE_ID>_<SLUG>_<DOC_TYPE>.md`，`<DOC_TYPE>` ∈ `DESIGN` | `IMPLEMENTATION` | `ROADMAP` | `REFACTOR_PLAN`

## 领域码

- 已启用：**仅 `CORE`**（框架内核与通用契约）。
- 其他子系统码在首个真实需求出现时按 `FEATURE_REGISTRY.md` 的追加规则登记；不得预置、不建空桶。

## 验证基线

- 当前 `docs-only`：没有构建与测试命令；验收 = 文档自检（占位符扫描、命名与 ID 合规、交叉引用指向真实路径）。
- 升级条件：首个代码 Feature 落地时升为 `tests-required`，在 `PROJECT_CONTEXT.md` 登记构建 / 测试命令，并同步 `WORKFLOW_PROFILE.md` 与本文件的这一节。

## 交接触发条件

当用户表示交接或切换会话时：
- 按 `templates/session-note.template.md` 写一份会话记录；
- 向 `PROGRESS_LOG.md` 追加一条；
- 未完成的 Slice 明确标为 Blocked / Deferred，写清原因与解除条件。
