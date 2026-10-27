# ITEM-F01 ItemDef / Instance / Frag 示例 — 薄设计

## Meta
- **ID:** `ITEM-F01`
- **Type:** Feature
- **Complexity:** L2
- **Status:** Done
- **Owner:** Max
- **Last updated:** 2026-10-27
- **Depends on:** `GPCONTENT-F01`（ContentDef / Frag / Registry）
- **Unlocks:** `INVENTORY-F01`
- **Related:** [Feature Registry](../FEATURE_REGISTRY.md)

## TL;DR

在通用 Content 之上特化 **Item**：`ItemDefFragmentBase` 做类型隔离、`ItemDef` + `ItemInstance`（stack + 可选 Property 袋）、示例片段、以及 **Weapon/Equipment 作为独立 ContentDef 再被 Item 片段引用**。示例 Def 关系写在无后缀 `ExampleItemDefs`（yaml-like），不编译、不做真实 DataAsset。

## Problem

需要可演示的「组合代 Item 继承」与 Def/Instance 分离；武器/防具等重数据不宜全塞进 ItemDef。

## Requirements

- `ItemDefFragmentBase`：仅服务 Item，防止 Frag 泄漏到无关 ContentDef。
- `ItemDef`：继承 ContentDef；`m_MaxStackCount` 为堆叠**唯一真源**（太常用，不做 StackableFragment，避免双份打架）；另有 description、category Tag 等。
- `ItemInstance`：def 引用、`m_StackCount`、Property 袋（可选设计：灵活换内存连续性；大批量游戏可改 union / 写死字段，注释必须写清取舍）。
- 示例片段：`Rarity` / `Consumable` / `Placeable` / `Equipable` / `Durability`（`m_MaxDurability` + `GrantProperty`）。
- **跨 Content 引用（新明确点）：**
  - `WeaponDef` / `EquipmentDef` 是独立 `GameplayContentDef`（自有数据多、不与 Item 常共享）。
  - `WeaponFragment` / `EquipmentFragment` 挂在 ItemDef 上，按名或指针引用对应 Def——**组合，不是 `WeaponDef : ItemDef`**。
- 示例清单：伪数据意思即可（见下）；GAS 接口留空。

## Non-Goals

- 真实 DataAsset / 编辑器资产。
- 完整武器战斗、装备槽玩法、工作台合成。
- Inventory 摆放逻辑（`INVENTORY-F01`）。

## 示例伪数据（示意，真源在 `ExampleItemDefs`）

```yaml
# 非编译；只表达 Def ↔ Frag ↔ 可选跨 Content 引用
- id: Stone
  maxStack: 64          # ItemDef.m_MaxStackCount 唯一真源

- id: Apple
  maxStack: 16
  fragments: [Consumable]

- id: IronPickaxe
  maxStack: 1
  fragments: [Durability]   # GrantProperty → 当前耐久在 Instance

- id: DirtBlock
  maxStack: 64
  fragments: [Placeable]

- id: IronHelmet
  fragments: [Equipable, Rarity, Durability]
  # 进阶示意（可选一行）：EquipmentFragment → EquipmentDef:IronHelmetStats

- id: IronSword
  fragments: [Durability]
  weapon: WeaponDef:IronSwordStats   # WeaponFragment 引用独立 WeaponDef
```

## 数据结构（点到为止）

```text
ItemDef : GameplayContentDef
  category, description, maxStackCount?, fragments…

ItemInstance
  def*, stackCount, properties[] (+ 运行时 name/tag 索引)

InstanceProperty*   // 可选袋；含 min/max/value（如 Int）
DurabilityFragment.GrantProperty → 写入 Instance
WeaponFragment → WeaponDef（独立 Catalog）
```

## 当前参考实现契约（钉死，避免歧义）

| 主题 | 契约 |
| --- | --- |
| Def 寿命 | `ItemInstance` 持有**非拥有** `ItemDef*`。典型由 Catalog/`shared_ptr` + Registry 常驻持有；不处理热重载/卸载顺序。 |
| 堆叠兼容 | `CanStackWith` **只比较** Def 指针 + 双方非空堆；**不比较** Instance 属性。带独立耐久等属性的示例多为 `maxStack=1`。 |
| 属性名 | 同一 Instance 内跨 Int/Float/Bool 桶**全局唯一**；重复 `AddProperty` 返回 false。 |
| Int range | 默认 `[INT_MIN, INT_MAX]`（无额外限制）；`SetRange` 后 clamp；`min>max` 时交换。 |
| Float range | 默认 min/max 为 **NaN = 该侧无限制**；仅对有限值夹取有界侧。 |
| 已实现类型 | 仅 Int / Float / Bool；枚举中其余值为占位，无 API。 |
| 跨 Content | Weapon/Equipment 为独立 ContentDef，由 Item 上 Fragment 引用（组合，非继承）。 |

## Open Questions

- Property 袋是否第一版就实现完整类型族，还是只做 Int/Float/Bool？— **已定：只做常用三种**，其余枚举占位。

## 正式化顺序

1. ItemDefFragmentBase / ItemDef / 示例 Frag（含 Durability Grant） — 完成  
2. InstanceProperty（常用三种）+ ItemInstance — 完成  
3. WeaponDef / EquipmentDef + 对应 Frag（骨架 + 中文注释） — 骨架完成  
4. 充实 `ExampleItemDefs` 伪数据；**不**加入 CMake 编译 — 完成  

## Design Review
- **Verdict:** Ready with deferred items（代码先行已正式化；薄设计作执行参照）
