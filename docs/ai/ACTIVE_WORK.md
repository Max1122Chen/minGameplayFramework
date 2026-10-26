# Active Work

人维护的短队列（建议同时 1–5 条）。

## 规则

- 只放可执行、当前有效的工作。
- 每条链接到已登记的 Feature ID。
- 状态保持新鲜；过期行移走或归档，不要留在表里当历史。

## 当前队列

| Priority | Feature ID | Slice ID | Status | Owner | Next action |
|----------|------------|----------|--------|-------|-------------|
| P0 | CORE-F01 | — | Review | 项目作者 | 评审 `docs/ai/CORE/CORE-F01_FRAMEWORK_LAYERING_DESIGN.md`：在第 11 节表格按「位置 + 意见」逐条填写；设计者随后回填判断依据与处理结论 |
| P1 | CORE-F06 | — | Planned（文档撰写中） | AI 协作 | 撰写会话与子系统 spec：对齐 C30–C36（子系统归属、初始化顺序、PIE 隔离、GI 注入点） |
| P1 | CORE-F03 | — | Planned（文档撰写中） | AI 协作 | 撰写内容定义 spec：对齐 C1–C14（稳定 id、引用形态、fragment 覆盖、只存增量、嵌套形态） |
| P2 | CORE-F02 | — | Planned（文档撰写中） | AI 协作 | 撰写属性与效果 spec：对齐 C6、C17–C21、C22–C29（基值/当前值、修饰器合成、三级失败策略） |
| P2 | CORE-F04 | — | Planned（文档撰写中） | AI 协作 | 撰写背包 spec：接口语义 + 三种容量策略 + 几何与槽位解耦；对齐 C17、C22、C28 |
| P2 | CORE-F05 | — | Planned（文档撰写中） | AI 协作 | 撰写图模型 spec：节点/边/payload 白名单/派生状态重算；对齐 C8、C11、C13 |
| P3 | CORE-F07 | — | Planned（可砍，文档撰写中） | AI 协作 | 撰写能力运行期 spec（可砍）：等 CORE-F02 与 CORE-F03 定稿后再写，砍掉时同步登记状态 |
| P3 | INFRA-F01 | — | Planned（待 F01 通过） | AI 协作 | 等 CORE-F01 评审通过后执行：CMake 工程、六个 target、doctest + ctest、验证基线升级为 tests-required |

说明：

- CORE-F02 ~ CORE-F07 的 spec 正在撰写中，状态为 Planned；撰写时逐条对照 CORE-F01 第 5 节契约，冲突处先改总纲。
- INFRA-F01 是唯一会引入代码与构建的条目；它落地时同时升级验证基线（AGENTS.md 第 9 节、PROJECT_CONTEXT.md 第 5 节、WORKFLOW_PROFILE.md）。
