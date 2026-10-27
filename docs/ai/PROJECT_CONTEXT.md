# 项目上下文

Last updated: 2026-10-27

## 1) 项目使命与成功判据

一句话使命：
- 沉淀作者个人的 GameplayFramework 设计与积累，**以设计思想为主体**；参考实现落在个人 C++ 引擎 minEngine 上，但设计与哲学必须能低摩擦迁移到其他引擎与语言。

成功判据：
- 设计可被第三方引擎 / 语言复用：去掉 C++ 与 minEngine 的具体 API 之后，这条设计依然成立。
- 参考实现能在 minEngine 上跑通：关键机制有可运行载体，不止于纸面。
- 稳定后的设计真相：探索期以作者代码 / 伪代码与口头契约为准；行为稳定后提炼设计文档与英文 Doxygen，此后「应该怎么做」以设计文档 + 代码契约注释为准，而不是记忆或对上游引擎的模仿。
- 迁移成本可验证：新增一个引擎 / 语言适配时，需要重写的是代码，不需要重新推导设计。

## 2) 架构方向

- 分层与模块边界**由作者在代码探索中逐步定型**，再提炼进 CORE 域设计文档；勿从已归档的 design v1 臆造模块。
- 已落地：`GPCONTENT-F01` / `ITEM-F01` / `INVENTORY-F01`（Done）。
- 要点：Def+Frag 组合；Item 可引用独立 Weapon/Equipment ContentDef；示例用无后缀伪数据，不做真实 DataAsset；Inventory 示例意思即可；Grid 旋转 = Frag Grant 的 Instance Property。
- 直接复用 minEngine：GameplayTag、EventBus、反射与序列化；Catalog 可挂 DataAsset 扩展点，本仓示例不强制走资产管线。
- 长期约束：设计与引擎解耦；能落到 minEngine C++，也能映射到其他引擎与语言。
- 统一依赖图模型 Deferred；具体树示例（如 SkillTree）可并行探索。

## 3) 当前阶段

- 协作骨架与 minEngine 产品工程已挂上；Content / Item / Inventory 正式化代码与测试已落地。
- CORE design v1 已作废（见 `docs/deprecated/`）；工作流为**代码 / 伪代码先行 → 审批 → Doxygen → 设计文档**。
- 下一批：SkillTree 等具体树示例（未纳入 Content/Inventory 提交）。

## 4) 协作约定

- 规划真相：`ACTIVE_WORK.md`、`FEATURE_REGISTRY.md`、`TECH_DEBT.md`、近期 `PROGRESS_LOG.md`，以及**作者正在写的源码草稿**。
- 探索期节奏见 `WORKING_WITH_AI.md`「探索期协作节奏」。
- 交付标准：遵循 `templates/DOC_GOVERNANCE.md`；空壳设计仍拒绝。
- 会话恢复：按 `BOOTSTRAP_DIGEST.md` 的读取顺序恢复上下文。
- 项目自有文档用中文；治理文档、`.cursor` 规则与技能保留英文原文。
- 源码注释：Doxygen；正式路径先中文（why / 解决问题 / 迁移注意）供审，通过后补英文，最终中英双语。禁止登记 ID 与设计章节号入注释。
- 设计文档保持薄，作执行参照；详细设计思想以代码注释为准（代码即文档）。

## 5) 验证基线

- 当前 `verification_bar` = **tests-required**。
- 构建：`cmake --build build --target MGF_Tests`（根 `CMakeLists.txt`；`ENGINE_ROOT` → minEngine）。
- 测试：`bin/MGF_Tests.exe`（CTest 名 `MGF_Tests`）。
- 文档变更仍须文档自检；代码变更须给出实际命令与输出。

## 6) 参考仓与只读边界

| 仓库 | 路径 | 用途 |
| --- | --- | --- |
| minEngine | `D:/Dev/GitRepo/minEngine` | 参考实现载体：类型系统与反射、序列化、AssetManager 等基础设施 |
| minCraft | `D:/Dev/GitRepo/minCraft` | minEngine 上的 gameplay 业务参考（itemDef & Instance、BlockDef 等） |
| min-agent-workflows | `D:/Dev/GitRepo/min-agent-workflows` | 本仓协作与治理文档的模板来源 |

- 三个参考仓**一律只读**：只借鉴，不修改、不提交、不格式化。
- 发现引擎能力缺口时，写在本仓的技术债或设计文档里，通过本仓的设计 / 适配层解决。
- 需要引擎侧改动时，只记录诉求与契约，不在参考仓就地改代码。

## 7) 文档拓扑

- `single-track`：产品决策与实现决策都记录在本 `docs/ai/` 树内，不存在第二套产品文档真相目录。
