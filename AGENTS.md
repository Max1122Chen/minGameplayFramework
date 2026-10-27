# minGameplayFramework

本仓是作者的个人 GameplayFramework 设计与积累。规范文档用中文；源码注释 Doxygen、中文先行供审，通过后补英文，最终中英双语。

## 1. 项目定位与愿景

- 本仓沉淀的是 **GameplayFramework 的设计思想与契约**，不是某个引擎的成品代码库。
- 参考实现载体：作者个人的 C++ 引擎 **minEngine**（`D:/Dev/GitRepo/minEngine`）。选择它的原因：C++ 侧不依赖大量独特语言特性；作者对该引擎极其熟悉；引擎较规整，并提供类型系统与反射、序列化、AssetManager 等基础设施，同时保持相对简单。
- 愿景：本仓的设计与积累能**低摩擦迁移到其他引擎与语言**。代码可以因语言重写，设计与哲学必须保留。
- 因此所有设计文档都必须能回答一个问题：去掉 C++ 与 minEngine 的具体 API 之后，这条设计是否仍然成立？

### 当前状态

- 协作与规范文档已就绪；minEngine 产品工程已挂上（CMake、`Source/minGameplayFramework/`）。
- **探索期工作流已翻转**：作者用伪代码 / 草稿代码表达设计；可落薄设计文档作执行参照；详细 why 写在代码注释里（代码即文档）。已归档的 CORE design v1 不得作实现依据（详见 `docs/ai/WORKING_WITH_AI.md`）。
- 领域码：`CORE`（预留）、`GPCONTENT`、`ITEM`、`INVENTORY`；活跃递进见 `FEATURE_REGISTRY.md`。

## 2. 目录与文件地图

```text
AGENTS.md                      本文件：agent 入口与长期规范（最高优先级的协作约束）
README.md                      对外说明：愿景、上手方式、边界
opencode.json                  opencode 配置：instructions 挂载 + 6 个命令
.gitignore                     忽略规则（已预置 C++ 构建产物，供未来代码工程使用）

docs/ai/                       协作真相与工作流记录（高层事实，不写实现细节）
  WORKFLOW_PROFILE.md          协作姿态（preset 与各维度取值），部署期决定，勿随手改
  PROJECT_CONTEXT.md           稳定项目快照：定位、技术约束、不变量
  BOOTSTRAP_DIGEST.md          会话恢复用的一页摘要
  WORKING_WITH_AI.md           AI 协作协议：读取顺序、产出边界、自检要求
  ACTIVE_WORK.md               活跃队列（当前为空骨架）
  FEATURE_REGISTRY.md          Feature 登记表与领域码表（当前仅 CORE）
  PROGRESS_LOG.md              追加式事实历史（只追加，不改写过去条目）
  TECH_DEBT.md                 显式技术债（Open / Closed）
  templates/                   设计与治理模板
    DOC_GOVERNANCE.md          文档类型、信任层级、Definition of Done
    WORKFLOW_PRESETS.md        preset 目录与维度→行为映射
    DESIGN_PRINCIPLES.md       设计原则
    design-spec.template.md    设计规格模板
    implementation-plan.template.md
    progress-log-entry.template.md
    session-note.template.md
    adr.template.md / bug-record.template.md / roadmap.template.md

.cursor/rules/                 Cursor 规则层（英文原文，从模板继承）
.opencode/skills/              opencode 技能层（英文原文，从模板继承）
```

设计长文放在 `docs/ai/<DOMAIN>/`（例如未来的 `docs/ai/CORE/`），不放在 `docs/ai/` 根目录；根目录只放协作真相。

## 3. 只读边界

以下三个仓库是**只读参考**，任何情况下不得修改、提交或格式化：

| 仓库 | 路径 | 用途 |
| --- | --- | --- |
| minEngine | `D:/Dev/GitRepo/minEngine` | 参考实现载体：类型系统、反射、序列化、AssetManager 等基础设施 |
| minCraft | `D:/Dev/GitRepo/minCraft` | minEngine 上的 gameplay 业务参考（itemDef & Instance、BlockDef 等） |
| min-agent-workflows | `D:/Dev/GitRepo/min-agent-workflows` | 本仓协作与治理文档的模板来源 |

- 发现引擎能力缺口时，写在**本仓**的技术债或设计文档里，通过本仓的设计/适配层解决。
- 需要引擎侧改动时，只记录诉求与契约，不在参考仓就地改代码。

## 4. 会话启动

### 读取顺序

新会话或上下文可能过期时，按此顺序恢复：

1. `docs/ai/WORKFLOW_PROFILE.md`
2. `docs/ai/PROJECT_CONTEXT.md`
3. `docs/ai/BOOTSTRAP_DIGEST.md`
4. `docs/ai/ACTIVE_WORK.md`
5. `docs/ai/FEATURE_REGISTRY.md`
6. `docs/ai/TECH_DEBT.md`
7. `docs/ai/PROGRESS_LOG.md` 最近若干条
8. 与当前任务相关的设计文档（`docs/ai/<DOMAIN>/`）

### profile 门禁

- `WORKFLOW_PROFILE.md` 的 `status` 为 `unconfigured` 时，按 `docs/ai/templates/WORKFLOW_PRESETS.md` **只问一次**元问题（现在配置 / 采用 `serious-engineering` 默认 / 稍后 `deferred`）。
- `status` 为 `configured`（本仓当前状态）时**不再追问**，直接按既有 profile 工作；只有用户明确说 `reconfigure workflow` 才重跑配置流程。
- profile 可以调整严格度、自治度、角色、语言、文档拓扑、验证基线、实现纪律。
- profile **不得**关闭：提交准备门禁（prepare≠execute）、规划信任层级、草稿≠大规模编码、注释禁 ID、分支禁 ID、L2+ 的质量 DoD。

### 规划信任层级

- 高信任（可据此排期）：`ACTIVE_WORK.md`、`FEATURE_REGISTRY.md`、`TECH_DEBT.md`、最近 `PROGRESS_LOG.md`、代码与测试/验证输出。
- 参考来源（不得自动变成 backlog）：旧 roadmap、标记为 Snapshot/Archived/Reference 的文档、历史设计文档里过期的勾选清单。
- 冲突时以运行期事实（代码、测试、验证输出）为准。

## 5. 注释规范（本仓重点）

这条规范的适用范围：**源码注释、日志字符串、测试用例名**。设计与协作文档不受此限（它们本就成体系地互相引用编号）。

### 5.1 代码即文档

- 优先用命名、类型、结构表达意图：名字没表达清楚的地方，先想能不能改名，而不是加注释。
- 公开类型与关键 API 的注释应能讲清：**为什么存在、解决什么问题、迁移到其他引擎时要注意什么**（与薄设计文档分工：文档管范围与清单，注释管就地契约）。
- 不要求逐字段、逐方法写注释。显而易见的注释是噪音，应删除。

### 5.2 风格：Doxygen；中文先行，再补英文

- 公开 API 用 `///`；跨行用 `/** ... */`。标签：`@brief`、`@param`、`@return`、`@warning`，必要时 `@note`。
- 内部实现用 `//`。
- **语言节奏**：正式化时先写**中文**契约注释供作者审；作者确认后再补**英文**，最终同一处中英双语各一份（可两行 `@brief` 或中英分段）。
- 探索期随手中文注可以保留到正式化时再规整语气。

```cpp
/// @brief 按句柄解析资源；尚未加载则加载。
/// @brief Resolves an asset handle to a live instance, loading it if needed.
/// @param handle 先前 acquire 返回的句柄，必须有效。 / Handle from acquire; must be valid.
/// @return 实例；加载失败则为 nullptr。 / The instance, or nullptr on load failure.
/// @warning 非线程安全。 / Not thread safe.
Asset* resolve(AssetHandle handle);

// 内部：按路径缓存，同一文件共享一次加载。
```

### 5.3 严禁出现的内容

注释、日志字符串、测试名里禁止出现：

- Feature / Slice / Bug / ADR / TechDebt 编号：`CORE-F01`、`CORE-F01-S02`、`TD-014`、`BUG-CORE-001`、`ADR-20261015-01`
- 裸切片标签：`S01:`、`F12`
- 设计文档章节号：`§3.4`、`design §2.7`、`H8`、`layer 2`
- 需求 / 判据编号：`O1`、`I1`、`Q3`
- 提交哈希
- 指向协作文档的指针：`see ACTIVE_WORK`、`as decided in the design doc`

理由：这些引用会腐坏。编号会被归档改写，章节会重排，提交会被 rebase 掉；引用一断，注释就从“信息”变成“负债”，读者对它的信任低于零。

### 5.4 正反例对照

反例：

```cpp
// §H8 (ii): the checkbox binds the AUTHORED flag.
// Implements CORE-F01-S02; see ACTIVE_WORK.
if (node->IsCheckbox()) node->SetFlag(Flag::Authored);
```

正例：

```cpp
// The checkbox binds the authored flag, so switching an ancestor off never
// rewrites the author's own choice.
if (node->IsCheckbox()) node->SetFlag(Flag::Authored);
```

反例（测试名）：

```cpp
TEST_CASE("CORE-F01-S02 overrides inherit") { ... }
```

正例：

```cpp
TEST_CASE("child override keeps ancestor default intact") { ... }
```

正确的做法始终是：**把“为什么”就地写清楚**，让读者不需要打开任何设计文档。

## 6. 命名与 ID 方案

协作文档里使用（仅在协作文档、分支之外的规划场景中使用，不进源码注释）：

- Feature ID：`<DOMAIN>-F<nn>`，例：`CORE-F01`
- Slice ID：`<FeatureID>-S<nn>`
- Bug ID：`BUG-<DOMAIN>-<nnn>`
- Tech Debt ID：`TD-<nnn>`
- ADR ID：`ADR-<yyyyMMdd>-<nn>`
- 长文档文件名（全大写）：`docs/ai/<DOMAIN>/<FEATURE_ID>_<SLUG>_<DOC_TYPE>.md`，`<DOC_TYPE>` ∈ `DESIGN` | `IMPLEMENTATION` | `ROADMAP` | `REFACTOR_PLAN`

### 领域码

- 已注册：`CORE`（预留）、`GPCONTENT`、`ITEM`、`INVENTORY`。
- 其他子系统（ABILITY、ATTR 等）**在首个真实需求出现时**才追加，不得预置。
- 追加规则：在 `FEATURE_REGISTRY.md` 的领域码表中登记；全大写；短；稳定（一旦有其他文档引用就不要改）；不建空桶。

## 7. 硬约束

### 提交门禁

- “prepare commit” 只等于：根据真实 diff 起草提交信息 + 通过就绪检查。
- 未获用户明确执行指令，**不得** `git commit`、push 或改写历史。

### 规划来源

- 只以 `ACTIVE_WORK.md` / `FEATURE_REGISTRY.md` / `TECH_DEBT.md` / 最近 `PROGRESS_LOG.md` / 代码与测试为规划来源。
- 参考文档、快照文档、历史设计里的过期清单不得自动变成待办。

### 重构纪律

- 重构必须指向明确的终态；“先留着，以后再收”的双路径不允许静默存在。
- 确实需要保留过渡路径时，在同一次变更里登记技术债，并写清退出条件。

### 分支命名

- 形式：`feat/<topic>` 或 `feat/<topic>/<subtopic>`，例：`feat/render`、`feat/render/material`。
- 禁止 ID 入名：`feat/core-f01-xxx` 不允许。

## 8. 复杂度分级与设计仪式

| 级别 | 判定 | 义务 |
| --- | --- | --- |
| L0 | 单文件或单点、需求无歧义、可回滚 | 直接做，无需设计文档 |
| L1 | 2–5 个文件、需求基本清晰 | 简短意图回放，然后实施 |
| L2 | 跨模块或多交付物、存在隐藏假设 | 默认：作者草稿 / 伪代码 → 正式代码审批 → 稳定后提炼 Design Spec；若明确设计先行则 Design → Review → Plan |
| L3 | 高不确定、跨子系统、需要探索 | 在 L2 之上做架构安全检查（所有权 / 契约 / 迁移 / 失败路径）；可用代码探索承担，不强制先写长文；空壳设计仍拒绝 |

- 分级先于行动：拿到任务先写一句“级别 + 理由”。
- 需求与方案要分开陈述：先把要解决的问题写清，再给候选方案。
- 设计长文用 `docs/ai/templates/design-spec.template.md`；评审用 `.opencode/skills/design-review/SKILL.md`，结论记为 Ready / Ready with deferred items / Not ready。
- 流程细节见 `docs/ai/templates/DOC_GOVERNANCE.md`；探索期节奏见 `docs/ai/WORKING_WITH_AI.md`。

## 9. 验证基线

- 当前 profile 的 `verification_bar` 是 **tests-required**：文档自检 + 可复现构建 / 测试。
- 构建：`cmake --build build --target MGF_Tests`（根 `CMakeLists.txt`；`ENGINE_ROOT` → minEngine）。
- 测试：`bin/MGF_Tests.exe`（CTest 名 `MGF_Tests`）。
- 每个可验证断言应有对应测试或可复现命令；代码改动须给出实际命令与输出，不接受“看起来对”。

## 10. Agent 适配层扩展规则

- 本仓当前保留的适配层：`AGENTS.md`、`docs/ai/`、`.opencode/skills/`、`opencode.json`、`.cursor/rules/`。
- 本仓明确**不**维护：`CLAUDE.md`、`.github/`（含 copilot-instructions）、`.clinerules`、`.windsurfrules`。
- 若将来要为某个工具新增适配层，规则是：**先改 `AGENTS.md`，再把内容同步成该工具的副本**。永远不要在多处各自演化同一份规则；`AGENTS.md` 是单一真相。
