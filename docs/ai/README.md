# AI 协作文档索引（docs/ai）

本目录是本仓人与 Agent 共享的协作真相。所有"接下来做什么"的判断从这里出发；
代码与命令输出是运行时真相，与文档冲突时以它为准，并回头修正文档。

## Agent：规划时必须读取的来源

1. `WORKFLOW_PROFILE.md` — 协作姿态（当前 `configured`，不再询问配置）
2. `PROJECT_CONTEXT.md` — 定位、使命、当前阶段
3. `ACTIVE_WORK.md` — 当前短队列
4. `FEATURE_REGISTRY.md` — 已登记 Feature（关注 In Progress / Planned）
5. `TECH_DEBT.md` — 未关闭的技术债
6. `PROGRESS_LOG.md` — 最近的事实性变更（只读近期条目）
7. 任务相关设计文档（由 ACTIVE_WORK 链接，或用户点名）

新会话按 `BOOTSTRAP_DIGEST.md` 走，它把顺序与门禁压在一页里。

## 信任层级

- **第 1 层｜运行时真相**：代码、命令输出、验证结果。与文档冲突时以它为准，并顺手修正文档。
- **第 2 层｜已确认决定**：`WORKFLOW_PROFILE.md`、`PROJECT_CONTEXT.md`、`FEATURE_REGISTRY.md` 中的登记状态、ADR。
- **第 3 层｜工作记录**：`ACTIVE_WORK.md`、`TECH_DEBT.md`、`PROGRESS_LOG.md`、会话记录。反映"当时"的状态，据以行动前先与第 1 层核对。
- **第 4 层｜模板与治理参考**：`templates/`。它规定"怎么写、怎么评"，不代表本仓现状。

冲突处理：高信任层级覆盖低信任层级；不得用第 4 层模板里的示例内容推断本仓待办。

## 核心文件

| 文件 | 作用 |
| --- | --- |
| `WORKFLOW_PROFILE.md` | 协作姿态：严格度、自主度、角色、语言、验证门槛；部署期决定，勿随手改 |
| `PROJECT_CONTEXT.md` | 稳定项目快照：使命、成功判据、架构方向、当前阶段、只读边界 |
| `BOOTSTRAP_DIGEST.md` | 会话恢复的一页摘要：读取顺序、门禁、硬规则、ID 方案 |
| `ACTIVE_WORK.md` | 人维护的短队列（当前为空队列） |
| `FEATURE_REGISTRY.md` | Feature 登记表与领域码表（当前仅 `CORE`，无条目） |
| `PROGRESS_LOG.md` | 追加式事实历史，不改写过去条目 |
| `TECH_DEBT.md` | 显式技术债与退出条件（当前无条目） |
| `WORKING_WITH_AI.md` | 协作习惯与常用提示词 |

## 模板区（templates/）

`templates/` 是从上游工作流仓逐字引入的英文治理文档与空白骨架，说明"怎么产出"而非"现在做什么"：

- 骨架：`adr.template.md`、`bug-record.template.md`、`design-spec.template.md`、`implementation-plan.template.md`、`progress-log-entry.template.md`、`roadmap.template.md`、`session-note.template.md`
- 治理：`DOC_GOVERNANCE.md`、`DESIGN_PRINCIPLES.md`、`WORKFLOW_PRESETS.md`、`README.md`

骨架里的日期与字段留白是模板本身的形态，不是本仓的未完成占位符；使用时应按实际内容填实。
本仓**不包含** `docs/ai/TMPL/`——那是上游模板仓自身的 Feature 设计档案，不得引入。

## 领域码与命名

- 当前已启用领域码：**仅 `CORE`**（框架内核与通用契约）；其他子系统在首个真实需求出现时按 `FEATURE_REGISTRY.md` 的规则追加。
- 设计长文放在 `docs/ai/<DOMAIN>/`，不放在本目录根下；根目录只放协作真相。
- 文件名全大写：`<FEATURE_ID>_<SLUG>_<DOC_TYPE>.md`，`<DOC_TYPE>` ∈ `DESIGN` | `IMPLEMENTATION` | `ROADMAP` | `REFACTOR_PLAN`。
- ID：Feature `<DOMAIN>-F<nn>`、Slice `<FeatureID>-S<nn>`、Bug `BUG-<DOMAIN>-<nnn>`、Tech Debt `TD-<nnn>`、ADR `ADR-<yyyyMMdd>-<nn>`。
- 完整规则见 `BOOTSTRAP_DIGEST.md` 与 `templates/DOC_GOVERNANCE.md`。
