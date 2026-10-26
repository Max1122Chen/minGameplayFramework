# 项目上下文

Last updated: 2026-10-26

## 1) 项目使命与成功判据

一句话使命：
- 沉淀作者个人的 GameplayFramework 设计与积累，**以设计思想为主体**；参考实现落在个人 C++ 引擎 minEngine 上，但设计与哲学必须能低摩擦迁移到其他引擎与语言。

成功判据：
- 设计可被第三方引擎 / 语言复用：去掉 C++ 与 minEngine 的具体 API 之后，这条设计依然成立。
- 参考实现能在 minEngine 上跑通：关键机制有可运行载体，不止于纸面。
- 文档即设计真相：判断"应该怎么做"的第一来源是设计文档，而不是注释、记忆或对上游引擎的模仿。
- 迁移成本可验证：新增一个引擎 / 语言适配时，需要重写的是代码，不需要重新推导设计。

## 2) 架构方向

- 框架的分层、模块边界与依赖方向**将由 CORE 域的设计文档确定；当前尚未设计，勿臆造模块**。
- 不预设子系统清单：ITEM、INVENTORY、ABILITY、ATTR 等在首个真实需求出现时才登记领域码与 Feature。
- 已知的长期约束：设计必须与引擎解耦，能落到 minEngine 的 C++ 实现，也能映射到其他引擎与语言。

## 3) 当前阶段

- 初始化完成：协作与规范文档骨架就位，**无代码工程、无构建系统、无 Feature**。
- 下一步里程碑：完成第一份 CORE 域设计文档（框架分层与通用契约），经设计评审后登记为 Feature。

## 4) 协作约定

- 规划真相：`ACTIVE_WORK.md`、`FEATURE_REGISTRY.md`、`TECH_DEBT.md`、近期 `PROGRESS_LOG.md`。
- 交付标准：遵循 `templates/DOC_GOVERNANCE.md` 的文档类型与 Definition of Done。
- 会话恢复：按 `BOOTSTRAP_DIGEST.md` 的读取顺序恢复上下文。
- 项目自有文档用中文；治理文档、`.cursor` 规则与技能保留英文原文。
- 源码注释用英文、Doxygen 风格；只写非显而易见的 why、契约、不变量、易错点，不逐字段逐方法注释。

## 5) 验证基线

- 当前 `verification_bar` = **docs-only**：没有构建命令、没有测试命令；验收手段是文档自检（占位符扫描、命名与 ID 合规、交叉引用指向真实路径）。
- 升级条件：首个代码 Feature 落地时升为 **tests-required**，届时在本节登记构建与测试命令，并同步 `WORKFLOW_PROFILE.md` 与 `BOOTSTRAP_DIGEST.md`。

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
