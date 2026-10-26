# Feature Registry

每个新 Feature 在开始实施规划之前必须先在此登记。

| Feature ID | Title | Domain | Status | Design Doc | Implementation Plan | Owner | Notes |
|------------|-------|--------|--------|------------|---------------------|-------|-------|
| CORE-F01 | 框架分层与跨块契约 | CORE | Review | docs/ai/CORE/CORE-F01_FRAMEWORK_LAYERING_DESIGN.md | — | 项目作者 | 总纲：分层、依赖方向、跨块契约、波次；待作者评审 |
| CORE-F02 | 属性、修饰器与效果（小 GAS 数值核） | CORE | Planned | docs/ai/CORE/CORE-F02_ATTRIBUTES_AND_EFFECTS_DESIGN.md | — | 项目作者 | 属性 + 修饰器 + 效果为核心；base/current 分离与全量重聚合 |
| CORE-F03 | 内容定义（ContentDef + Fragment + 注册表） | CORE | Planned | docs/ai/CORE/CORE-F03_CONTENT_DEFINITION_DESIGN.md | — | 项目作者 | 内容定义、片段、实例增量、注册表与加载期校验 |
| CORE-F04 | 背包（Inventory） | CORE | Planned | docs/ai/CORE/CORE-F04_INVENTORY_DESIGN.md | — | 项目作者 | 基类只给接口语义，容量策略可派生；几何与槽位解耦 |
| CORE-F05 | 图模型（Graph） | CORE | Planned | docs/ai/CORE/CORE-F05_GRAPH_MODEL_DESIGN.md | — | 项目作者 | 节点 + 边 + 多态 payload + 可丢弃派生状态 |
| CORE-F06 | 会话与子系统（GameInstance 注入） | CORE | Planned | docs/ai/CORE/CORE-F06_SESSION_AND_SUBSYSTEMS_DESIGN.md | — | 项目作者 | 复用引擎 GameInstance / Subsystem，不造并行单例 |
| CORE-F07 | 能力运行期（小 GA，可砍） | CORE | Planned | docs/ai/CORE/CORE-F07_ABILITY_RUNTIME_DESIGN.md | — | 项目作者 | 可砍：能力编排、目标侧准入、层数上限 |
| INFRA-F01 | 工程骨架与验证基线 | INFRA | Planned | docs/ai/INFRA/INFRA-F01_PROJECT_SCAFFOLD_DESIGN.md | — | 项目作者 | 工程骨架、构建与验证；待 CORE-F01 评审通过后执行 |

## 已启用领域码

| 领域码 | 含义 | 状态 |
| --- | --- | --- |
| `CORE` | 框架内核与通用契约 | 已启用 |
| `INFRA` | 工程骨架、构建与验证 | 已启用 |

`INFRA` 单独成域的理由：工程骨架、构建系统与验证基线不属于 gameplay 内核，把它们塞进 CORE 会让「框架内核」这个概念同时承担产品设计与工程流水线两种变化原因。

其他子系统（ITEM、INVENTORY、ABILITY、ATTR 等）**不得预置**：没有真实需求就不建码，也不建空桶。

## 如何追加领域码

- **谁决定**：用户确认；Agent 不自行开新域，只提出建议。
- **写哪里**：本文件的领域码表 + `BOOTSTRAP_DIGEST.md` 的「领域码」节，同一次变更内一起改。
- **命名约束**：全大写、短、稳定；不复用已退役的码，不改名已被引用的码，不为"以后可能用到"而预注册。

## 状态说明

- `Draft`：概念存在，尚不可实施
- `Planned`：已批准实施
- `In Progress`：进行中
- `Review`：等待验证 / 评审
- `Done`：已验收
- `Blocked` / `Deferred` / `Cancelled`：必须在相关文档里写清原因与后续条件

## 追加规则

- 先登记，再规划，再实施；未登记的 Feature 不得进入实施规划。
- Design Doc / Implementation Plan 两列填真实路径，路径随文档创建而更新。
- Status 变化时，同一次变更内同步 `ACTIVE_WORK.md`，并在 `PROGRESS_LOG.md` 追加一条。
- 可砍 Feature（当前为 CORE-F07）在 Notes 里写明「可砍」，砍掉时状态改 `Cancelled` 并写清原因与解除条件。
