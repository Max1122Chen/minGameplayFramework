# INVENTORY-F01 薄接口与三类背包示例 — 薄设计

## Meta
- **ID:** `INVENTORY-F01`
- **Type:** Feature
- **Complexity:** L2
- **Status:** Done
- **Owner:** Max
- **Last updated:** 2026-10-27
- **Depends on:** `ITEM-F01`（ItemInstance / ItemDef / 相关 Frag）
- **Related:** [Feature Registry](../FEATURE_REGISTRY.md)

## TL;DR

`IInventoryBase` 只保留最通用的 TryAdd/TryRemove；用 **SlotBased / CategoryBased / GridBased** 三类**示例**演示常见形态。例子意思即可（Chest 可编译骨架；Workbench/Furnace 注释示意，不做 MC 合成/烧炼）。Grid：**占格 + 旋转**；旋转由 `GridGeometryFragment` 片段 **Grant 到 ItemInstance 的 Property**，而非写在 Def 上当可变真源。

## Problem

背包业务差异大，不能也不该做一个万能 Inventory；需要可迁移的薄接口 + 可读示例。

## Requirements

- `IInventoryBase`：`TryAddItem` / `TryRemoveItem`；入参 stack **就地扣减**，返回实际数量。
- **SlotBased：** 定长槽向量，下标即 slot；Minecraft 风格 `PlayerInventory` 用区间常量意思一下；`Chest` 骨架；Workbench/Furnace **不实现玩法**。
- **CategoryBased：** 按 Tag 路由到子容器（Skyrim/FO4/BotW 叙事）。
- **GridBased：** 塔科夫 / RE4 / PZ 式占格；`GridGeometryFragment` 定义 w/h，并 **Grant 旋转相关 Instance Property**（放置态）；支持 90° 旋转；简单整理 = 可堆叠合并 + First-Fit（含旋转尝试）。Frag 放在 Grid 示例目录旁，便于对照阅读。
- minCraft 仅供参考，API **不必对齐**。

## Non-Goals

- 工作台合成、熔炉烧炼、嵌套容器、最优装箱、完整 UI。
- 把分类/网格 API 塞进 `IInventoryBase`。

## 数据结构（点到为止）

```text
IInventoryBase
  TryAdd / TryRemove  →  mutate input stack, return accepted count

SlotBasedInventory
  capacity, items[slot]

GridGeometryFragment (on ItemDef)
  gridW, gridH
  GrantProperty → Instance 上的旋转（等）Property

GridBasedInventory
  placed items（含 origin；朝向读 Instance Property 或放置记录——正式化时与 Grant 方案一致，优先 Property）
```

## 旋转语义（已定）

- Def 侧 Frag 声明「可旋转 / 默认朝向 / 占格尺寸」。
- **当前朝向**是 Instance（或等价放置态）上的 Property，由 Frag `GrantProperty` 赋予——与 Durability 同一模式。
- 整理与 FindFirstFit 读取该 Property + w/h 计算占用。

## Open Questions

- 放置 origin (x,y) 放在 Inventory 侧放置记录，还是也做成 Instance Property？— **建议 origin 在 Inventory 放置记录**（同一物品换包不应绑死坐标）；旋转用 Instance Property。非 blocking。

## 正式化顺序

1. `IInventoryBase` 契约注释  
2. SlotBased + Player 区间示意 + Chest；Workbench/Furnace 空类或注释  
3. CategoryBased  
4. GridGeometryFragment（Grant 旋转 Property）+ GridBased（旋转放置 + 简单整理）  
5. 少量单测意思一下（若验证基线已升档）  

## Design Review
- **Verdict:** Ready with deferred items（代码先行已正式化；薄设计作执行参照）
