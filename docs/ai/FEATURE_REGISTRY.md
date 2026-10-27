# Feature Registry

每个新 Feature 在开始实施规划之前必须先在此登记。

| Feature ID | Title | Domain | Status | Design Doc | Implementation Plan | Owner | Notes |
|------------|-------|--------|--------|------------|---------------------|-------|-------|
| GPCONTENT-F01 | ContentDef / Frag / Catalog / Registry | GPCONTENT | Done | [DESIGN](GPCONTENT/GPCONTENT-F01_CONTENT_DEF_FRAG_REGISTRY_DESIGN.md) | — | Max | 正式化代码 + 反射；薄设计作执行参照 |
| ITEM-F01 | ItemDef / Instance / 示例与跨 Content 引用 | ITEM | Done | [DESIGN](ITEM/ITEM-F01_ITEM_DEF_INSTANCE_AND_EXAMPLES_DESIGN.md) | — | Max | maxStack 真源在 ItemDef；`ExampleItemDefs` 伪数据 |
| INVENTORY-F01 | 薄 Inventory + 三类示例族 | INVENTORY | Done | [DESIGN](INVENTORY/INVENTORY-F01_INVENTORY_FAMILIES_DESIGN.md) | — | Max | Slot/Category/Grid；委托事件；doctest |
| SKILL-F01 | SkillTree Def / Instance / Builder 示例 | SKILL | Review | — | — | Max | 代码先行；费用在树节点；Builder 做 DAG；反射已挂，待作者再审 |

已作废：`CORE-F01` 合并稿（拆入上表 GPCONTENT/ITEM/INVENTORY）；勿再引用。

## 递进关系

```text
GPCONTENT-F01  →  ITEM-F01  →  INVENTORY-F01   （已 Done）
SkillTree 示例（SKILL-F01）并行；统一 Graph 抽象 Deferred
```

## 已启用领域码

| 领域码 | 含义 | 状态 |
| --- | --- | --- |
| `CORE` | 框架内核与通用契约（预留；当前无活跃 Feature） | 已启用 |
| `GPCONTENT` | GameplayContent：Def / Fragment / Catalog / Registry | 已启用 |
| `ITEM` | 物品定义、实例、物品侧 Frag 与示例 | 已启用 |
| `INVENTORY` | 背包薄接口与示例族 | 已启用 |
| `SKILL` | 技能树等技能内容示例（具体树先行，非统一 Graph） | 已启用 |

其他子系统（ABILITY、ATTR 等）**不得预置**：没有真实需求就不建码，也不建空桶。

## 如何追加领域码

- **谁决定**：用户确认；Agent 不自行开新域，只提出建议。
- **写哪里**：本文件的领域码表 + `BOOTSTRAP_DIGEST.md` 的「领域码」节，同一次变更内一起改。
- **命名约束**：全大写、短、稳定；不复用已退役的码，不改名已被引用的码，不为"以后可能用到"而预注册。

## 状态说明

- `Draft`：概念存在，尚不可实施
- `Planned`：已批准计划
- `In Progress`：进行中
- `Review`：等待验证 / 评审
- `Done`：已验收
- `Blocked` / `Deferred` / `Cancelled`：必须在相关文档里写清原因与后续条件
