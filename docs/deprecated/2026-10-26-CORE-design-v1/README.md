# CORE 域索引

CORE 域承载框架内核与通用契约：不面向某一款游戏，而是所有 gameplay 模块共处的规则面。工程骨架与构建验证不属于 gameplay 内核，放在 INFRA 域。

## Feature 一览

| 编号 | 标题 | 文档路径 | 状态 | 依赖 |
| --- | --- | --- | --- | --- |
| CORE-F01 | 框架分层与跨块契约（总纲） | `docs/ai/CORE/CORE-F01_FRAMEWORK_LAYERING_DESIGN.md` | Review（待作者评审） | 无（本域根文档） |
| CORE-F02 | 属性、修饰器与效果（小 GAS 数值核） | `docs/ai/CORE/CORE-F02_ATTRIBUTES_AND_EFFECTS_DESIGN.md` | Planned（文档撰写中） | CORE-F01 |
| CORE-F03 | 内容定义（ContentDef + Fragment + 注册表） | `docs/ai/CORE/CORE-F03_CONTENT_DEFINITION_DESIGN.md` | Planned（文档撰写中） | CORE-F01 |
| CORE-F04 | 背包（Inventory） | `docs/ai/CORE/CORE-F04_INVENTORY_DESIGN.md` | Planned（文档撰写中） | CORE-F01、CORE-F03 |
| CORE-F05 | 图模型（Graph） | `docs/ai/CORE/CORE-F05_GRAPH_MODEL_DESIGN.md` | Planned（文档撰写中） | CORE-F01、CORE-F03 |
| CORE-F06 | 会话与子系统（GameInstance 注入） | `docs/ai/CORE/CORE-F06_SESSION_AND_SUBSYSTEMS_DESIGN.md` | Planned（文档撰写中） | CORE-F01 |
| CORE-F07 | 能力运行期（小 GA，可砍） | `docs/ai/CORE/CORE-F07_ABILITY_RUNTIME_DESIGN.md` | Planned（可砍） | CORE-F01、CORE-F02、CORE-F03 |

INFRA 域的对偶条目：INFRA-F01 工程骨架与验证基线，`docs/ai/INFRA/INFRA-F01_PROJECT_SCAFFOLD_DESIGN.md`，状态 Planned，依赖 CORE-F01 评审通过。

## 文档命名

- 文件名全大写：`<FEATURE_ID>_<SLUG>_<DOC_TYPE>.md`，本域全部为 `DESIGN` 类型。
- 长文档只放在 `docs/ai/CORE/`，不放 `docs/ai/` 根目录（根目录只放协作真相）。
- 一份框架总纲（CORE-F01）+ 每个 Feature 一份 Design Spec；总纲承载跨块契约，其余 spec 只写本 Feature 在其约束下的具体形态。

## 评审区约定

- 设计文档标题必须带编号（H1 以 Feature 编号开头），章节标题全部带编号，便于评审按位置提意见。
- 每份设计文档末尾固定两节：**待决问题**（编号 Q1..Qn，每条给选项与建议）与**评审意见（作者填写）**表。
- 作者只写「位置 + 意见」；判断依据与处理结论由设计者回填；处理结论三选一：接受 / 反驳 + 理由 / 转待决问题。
- 其余六份 spec 必须逐条对照 CORE-F01 第 5 节的契约条目，冲突处回来改总纲，不允许单点例外。
