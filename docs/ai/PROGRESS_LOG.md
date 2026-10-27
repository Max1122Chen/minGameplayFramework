# Progress Log

按时间顺序追加的项目记录，只追加，不改写过去条目。

## 2026-10-26

- 范围：仓库初始化（工作流与文档骨架），无 Feature ID。
- 完成：
  - 建立 `docs/ai/` 协作层：画像、上下文、bootstrap、队列、登记表、进度、技术债、协作习惯、索引
  - 从上游工作流模板仓逐字引入 `docs/ai/templates/` 的骨架与治理文档（英文原文，逐字节一致）
  - 固化工作流画像：预设 `serious-engineering`、自主度 `propose-then-act`、角色 `partner`、
    文档语言 `zh`、`single-track`、`implementation_discipline` 开启
- 决策要点：
  - 验证基线暂定 `docs-only`（无构建、无测试）；首个代码 Feature 落地时升为 `tests-required`
  - 领域码只登记 `CORE`；其他子系统等首个真实需求出现再追加，不预置
  - 适配层取舍：保留 AGENTS.md、docs/ai/、.opencode/skills/、opencode.json、.cursor/rules/；
    不创建 CLAUDE.md、.github/、.clinerules、.windsurfrules
  - 不引入上游的 `docs/ai/TMPL/`（那是模板仓自身的 Feature 档案）
  - 源码注释用英文 Doxygen 风格，且禁止出现 Feature / Slice / Bug / ADR 编号与设计章节号
- 验证：
  - 文档自检：占位符扫描、命名与 ID 合规、交叉引用指向真实路径、模板文件与上游逐字节一致
- 版本控制：
  - 本仓此前不是 git 仓库；本次 `git init`（默认分支 `main`），远端 `origin` 设为
    `https://github.com/Max1122Chen/minGameplayFramework.git`，初始化提交已推送
  - 加入 `.gitattributes`（`* text=auto eol=lf`）并标记常见二进制类型，
    消除 Windows 检出时的整文件换行假 diff
- 下一步：
  - 起草 CORE 域第一份设计文档（框架分层与通用契约），评审通过后登记 Feature

## 2026-10-26（方向调整：设计 v1 归档，改为作者亲手写代码）

- 决定：作者不认可第一批设计的具体取向。设计改由作者本人写代码或伪代码表达，agent 以代码为设计输入。
- 处理：
  - 七份设计文档（总纲 + 六份 spec）整体移入 docs/deprecated/2026-10-26-CORE-design-v1/，逐份加作废横幅
  - docs/ai/CORE/ 删除；FEATURE_REGISTRY.md、ACTIVE_WORK.md、PROGRESS_LOG.md 恢复到设计周期开始前的状态
  - 新增 docs/deprecated/README.md，写明归档件不得作为实现依据或排期来源
- 不变：AGENTS.md 的协作纪律（提交门禁、注释禁 ID 与章节号、信任层级、复杂度分级）继续有效

## 2026-10-26（固化「代码先行」协作翻转 + Content/Inventory 草稿阅读）

- 范围：协作真相同步；未改业务源码。
- 完成：
  - 将探索期路径写入 `WORKING_WITH_AI.md`、`PROJECT_CONTEXT.md`、`BOOTSTRAP_DIGEST.md`、`WORKFLOW_PROFILE.md` Effective behavior、`AGENTS.md` §8 / 当前状态、`ACTIVE_WORK.md` 说明
  - 默认路径：作者伪代码/草稿 → agent 正式化与审批 → Doxygen → 提炼设计文档；空壳设计仍拒绝
- 作者草稿阅读理解（非正式定稿）：
  - `GameplayContentDef` + `GameplayContentFragment`：组合替代 ContentDef 继承；推荐 Def=Schema、Instance=可变值
  - `ItemDef` / 示例 Frag（Stackable、Rarity、Consumable、Placeable、Equipable、Durability）与 `ItemInstance` 属性袋
  - `IInventoryBase` 薄接口；SlotBased / GridBased 为示例实现；`GridGeometryFragment` 挂在 grid 示例侧
- 下一步：按作者指定切片正式化代码（include / 反射 / 存储字段等），或继续补伪代码

## 2026-10-26（注释规范 + CORE-F01 薄设计 Draft）

- 范围：协作规范与 CORE-F01 设计文档；未正式化业务源码。
- 完成：
  - 注释节奏改为：正式化先中文 Doxygen（why / 问题 / 迁移）→ 作者审过再补英文 → 最终双语；同步 `AGENTS.md`、`WORKING_WITH_AI.md`、`.cursor/rules/code-comments-no-feature-ids.mdc` 等
  - 设计文档要求：薄执行参照；详细思想在代码注释（代码即文档）
  - 新增 `docs/ai/CORE/CORE-F01_GAMEPLAY_CONTENT_AND_INVENTORY_EXAMPLES_DESIGN.md`（Draft）；登记 FEATURE_REGISTRY / ACTIVE_WORK
  - Item 示例提议：Stone / Apple / IronPickaxe / DirtBlock / IronHelmet；Grid 含旋转 + 合并后 First-Fit 整理
- 下一步：作者 review 设计文档并拍板 Open Questions，然后按文档 checklist 正式化

## 2026-10-26（正式化代码进行中；Fragment 中文统一为「片段」）

- 废弃 CORE design v1 已提交：`e161773`
- 作者 `Source/` 草稿保留在暂存区；正式化改动与新增 `.cpp` 保持未暂存，便于 diff 对比
- 约定：`ItemDef.m_MaxStackCount` 为堆叠唯一真源（无 StackableFragment）；Fragment 中文称「片段」
- 已铺开：ContentDef/Registry、ItemDef/Instance/示例片段、Weapon/Equipment、三类 Inventory 示例（含 Grid 旋转 Property）

## 2026-10-26（领域拆分 GPCONTENT/ITEM/INVENTORY + 设计对齐）


- 范围：按作者反馈改领域与设计；充实伪示例；未正式化编译代码。
- 决定：
  - 领域细化：`GPCONTENT` → `ITEM` → `INVENTORY` 递进；作废合并的 `CORE-F01` 稿
  - 示例不做真实 minEngine DataAsset；用无后缀 `ExampleItemDefs` 等 yaml-like 伪数据
  - Grid 旋转：`GridGeometryFragment` Grant 到 ItemInstance 的 Property
  - Workbench/Furnace 等只意思一下，不做 MC 玩法
- 完成：三份薄设计 + 登记表/ACTIVE_WORK/领域码同步；`ExampleItemDefs` 写入示意关系（含 WeaponFragment 引用独立 WeaponDef）
- 作者草稿增量已对齐：Content 级 id/name/fragments 存储、Catalog/Registry、Item 常用字段可上 Def、Property 袋为可选设计、Weapon/Equipment 独立 ContentDef
- 下一步：作者 review 三份 Draft；通过后按递进正式化

## 2026-10-27（GPCONTENT / ITEM / INVENTORY 正式化落地）

- 范围：三域代码正式化 + 薄设计状态同步；不含 GameplayDataGraph / SkillTree。
- 完成：
  - `GameplayContentDef` / Fragment / Catalog / Registry；`ItemDef`（maxStack 真源）/ `ItemInstance` / 示例片段；Weapon/Equipment 独立 Def
  - Inventory：`IInventoryBase` + Slot / Category / Grid 示例（委托事件、缩容整理、Grid 旋转 Property）
  - `MGF_Tests`（doctest）覆盖 ItemInstance 与三类背包；产品 CMake + Game/Editor 宿主
  - 薄设计三份 Status → Done；登记表 Done；ACTIVE_WORK 清空该递进
- 验证：`cmake --build build --target MGF_Tests`；`bin/MGF_Tests.exe` 相关用例通过
- 验证基线：升为 `tests-required`（见 `WORKFLOW_PROFILE.md` / `PROJECT_CONTEXT.md` / `BOOTSTRAP_DIGEST.md`）
- 下一步：SkillTree 示例正式化与反射挂接（工作区未纳入本次提交）
