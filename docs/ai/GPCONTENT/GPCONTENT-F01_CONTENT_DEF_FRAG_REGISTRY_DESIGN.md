# GPCONTENT-F01 ContentDef / Fragment / Catalog / Registry — 薄设计

## Meta
- **ID:** `GPCONTENT-F01`
- **Type:** Feature
- **Complexity:** L2
- **Status:** Done
- **Owner:** Max
- **Last updated:** 2026-10-27
- **Depends on:** —
- **Unlocks:** `ITEM-F01`、后续其他 Content 域
- **Related:** [Feature Registry](../FEATURE_REGISTRY.md)

## TL;DR

定游戏内容的通用骨架：**薄 Def + Fragment 组合**、Def/Instance 分工、按目录聚合的 Catalog、挂在 GameInstance 上的 Registry。类型形状以作者 `GameplayContentDef.hpp` / `GameplayContentRegistry.hpp` 为准（代码即文档）。本仓示例**不**做成真实 minEngine DataAsset 资产管线，关系用无后缀伪数据文件或注释示意。

## Problem

缺少可迁移的「内容定义」层：组合代继承、可查询 Frag、目录与注册，供 Item / Weapon / Equipment 等递进特化。

## Requirements

- `GameplayContentFragment`：Schema 片段（Fragment）；身份可用 string / Tag / `MEClass`（多键为可迁移参考，具体引擎可裁）。
- `GameplayContentDef`：持有 `m_Fragments` + `m_ContentId` / `m_ContentName`；`GetFragment` 族。
- **推荐：** Frag 只声明值域 / 要 Grant 哪些 Instance Property；可变值在 Instance。运行期改 Frag 字段可行但不提倡。
- `GameplayContentCatalog`：一类内容的 Def 列表（引擎侧可继承 DataAsset；**本仓示例用伪数据表达**，不强制落盘 `.measset`）。
- `GameplayContentRegistry`：GameInstanceSubsystem，持有多个 Catalog + 查找表。

## Non-Goals

- 具体 Item / 背包业务（见 `ITEM-*` / `INVENTORY-*`）。
- 本仓内完整 AssetManager 编资产、编辑器制作流程。

## 数据结构（点到为止）

```text
GameplayContentDef
  id, name, fragments[]

GameplayContentCatalog   // 逻辑上 = 某类 Def 的表；引擎可挂 DataAsset
  tag, name, defs[]

GameplayContentRegistry  // GameInstance 子系统
  catalogs[] + lookup maps
```

## 与引擎的关系

- 正式游戏可用 DataAsset 承载 Catalog；框架代码保留该扩展点。
- **本仓库示例路径：** yaml/json/md-like 伪代码（如后续 Item 的 `ExampleItemDefs` 无后缀文件），**不参与编译**。

## 身份键分工（参考实现）

| 键 | 用途 |
| --- | --- |
| `MEClass` / `GetFragment<T>()` | **运行时类型身份**（同类型建议至多一个；`AddFragment` 拒绝重复类型） |
| `GetFragmentId()` string / `GetFragmentIdTag()` | **可迁移查找键**；参考实现并存，迁移到具体项目时应二选一 |
| GameplayTag on Def/Item | **玩法分类**（如物品类别路由），不是 Fragment 类型替代品 |

不要把 string / Tag / 反射类型当成可互换的“万能 ID”。

## Open Questions

- Catalog 查找键：id 与 name 并存是否足够？— 非 blocking；碰撞时后注册覆盖（当前行为）。

## 正式化顺序

1. Fragment / Def（含 Frag 存储与查询）+ 中文注释 — 完成  
2. Catalog / Registry 骨架 + GameInstance 挂点 — 完成  
3. 不写真实示例资产；仅在注释中示意 Catalog 装载关系 — 完成  

## Design Review
- **Verdict:** Ready with deferred items（代码先行已正式化；薄设计作执行参照，详细契约在源码注释）
