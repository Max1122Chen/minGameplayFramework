# minGameplayFramework

个人 GameplayFramework 的**设计与积累**。不是成品框架，而是一份可以带走、可以迁移、可以按需实现的 gameplay 设计资产。

## 这个仓库是什么

游戏 gameplay 层（属性、技能、物品、背包、伤害、状态、存档契约……）的真正难点很少在代码量，而在**契约与不变量**：谁持有真值、谁负责推导、什么时候可以缓存、边界在哪一层。

引擎会换、语言会换，但这些判断不会。本仓把这类设计沉淀成可读、可评审、可交接的文档：每条设计讲清楚它解决什么问题、放弃了什么、依赖哪些不变量、迁移到别的引擎时要重写哪一部分。

一句话：**设计优先于实现，思想可迁移，实现可重写。**

## 为什么以 minEngine 为参考实现载体

1. **没有语言特性依赖**：C++ 侧不依赖大量独特语言特性，设计不会被某个语言的“魔法”绑架。
2. **作者极其熟悉**：讨论能直接落到具体 API，不必猜引擎行为。
3. **基础设施够用且简单**：提供类型系统与反射、序列化、AssetManager 等 gameplay 必需的地基，同时保持相对简单，适合做参考实现而不至于淹没设计本身。

定位因此是：**代码可因语言重写，设计与哲学必须保留**。任何设计文档都要能回答“去掉 C++ 与 minEngine API 之后，这条设计是否仍然成立”。

## 如何开始

### 新会话 bootstrap

把下面这段直接粘给 AI 助手（opencode / Cursor 或任意能读文件的 agent）：

```text
Read AGENTS.md, then docs/ai/WORKFLOW_PROFILE.md, docs/ai/PROJECT_CONTEXT.md,
docs/ai/BOOTSTRAP_DIGEST.md, docs/ai/ACTIVE_WORK.md, docs/ai/FEATURE_REGISTRY.md
and the latest entries of docs/ai/PROGRESS_LOG.md.
Summarize: current posture, what is active, what is blocked, and propose the single
next concrete step with its verification command. Do not start coding before summarizing.
```

### 目录导览

| 路径 | 作用 |
| --- | --- |
| `AGENTS.md` | agent 入口与长期规范：定位、只读边界、读取顺序、注释规范、命名、硬约束、分级、验证基线 |
| `README.md` | 本文件：对外说明与上手方式 |
| `opencode.json` | opencode 配置：挂载 instructions，定义 6 个命令 |
| `.gitignore` | 忽略规则（已预置 C++ 构建产物） |
| `docs/ai/PROJECT_CONTEXT.md` | 稳定项目快照与不变量 |
| `docs/ai/WORKFLOW_PROFILE.md` | 协作姿态（preset 与各维度取值） |
| `docs/ai/BOOTSTRAP_DIGEST.md` | 一页式会话恢复摘要 |
| `docs/ai/ACTIVE_WORK.md` | 活跃队列 |
| `docs/ai/FEATURE_REGISTRY.md` | Feature 登记表与领域码表（`CORE` / `GPCONTENT` / `ITEM` / `INVENTORY` / `SKILL`） |
| `docs/ai/PROGRESS_LOG.md` | 追加式事实历史 |
| `docs/ai/TECH_DEBT.md` | 显式技术债 |
| `docs/ai/WORKING_WITH_AI.md` | AI 协作协议 |
| `docs/ai/templates/` | 设计、计划、ADR、bug、会话记录模板与治理文档 |
| `.cursor/rules/` | Cursor 规则层（英文原文） |
| `.opencode/skills/` | opencode 技能层（英文原文） |

### 登记一个 Feature

1. 先判断复杂度（L0–L3，定义见 `AGENTS.md` 第 8 节）。
2. L2+ 先在 `docs/ai/FEATURE_REGISTRY.md` 里用 `<DOMAIN>-F<nn>` 登记（例如 `CORE-F01`），并链接设计文档路径。
3. 设计文档写到 `docs/ai/<DOMAIN>/`，文件名全大写：`<FEATURE_ID>_<SLUG>_DESIGN.md`。
4. 把在办项写进 `docs/ai/ACTIVE_WORK.md`；只有它能作为排期依据。

### 追加领域码

已启用：`CORE`（预留）、`GPCONTENT`、`ITEM`、`INVENTORY`、`SKILL`。ABILITY、ATTR 等仍**不预置**：等第一个真实需求出现时再到 `FEATURE_REGISTRY.md` 追加——全大写、短、稳定，不建空桶。

### 命令（opencode.json 中定义）

| 命令 | 用途 |
| --- | --- |
| `bootstrap` | 会话开始：恢复上下文并给出下一步 |
| `reconfigure-workflow` | 重新配置协作姿态（preset 或自定义维度） |
| `design` | 分级 → 发现既有架构 → 写设计规格 |
| `design-review` | 评审设计，给出 Ready / Ready with deferred items / Not ready |
| `handoff` | 会话结束：写 PROGRESS_LOG、标记阻塞项、给下一步首个动作 |
| `register-feature` | 在 FEATURE_REGISTRY 登记新 Feature ID |

## 写作与代码规范摘要

- 项目自有文档用**中文**；从模板继承的治理文档、`.cursor` 规则、技能保留**英文**原文。
- 源码注释：Doxygen；正式化先**中文**契约供审，通过后补**英文**，最终中英双语。公开 API 用 `///`（`@brief` / `@param` / `@return` / `@warning`），内部用 `//`。
- **不逐字段、逐方法写注释**：只写非显而易见的 why、契约、不变量、前置条件、易错点。
- 注释、日志、测试名里**禁止**出现 Feature/Slice/Bug/ADR/TechDebt 编号、裸切片标签、设计文档章节号、需求编号、提交哈希、指向协作文档的指针——把“为什么”就地写清楚。
- 分支命名 `feat/<topic>[/<subtopic>]`，禁止 ID 入名。

完整规范见 `AGENTS.md` 第 5 节。

## 当前状态

- 协作文档 + minEngine 产品工程（CMake、`Source/minGameplayFramework/`、`MGF_Tests`）已挂上。
- **已落地（有代码与测试）**：Gameplay Content（Def / Fragment / Catalog / Registry）、ItemDef / ItemInstance、三类 Inventory 示例（Slot / Category / Grid）。
- **Review 中**：SkillTree 示例（`SKILL-F01`）；统一 `GameplayDataGraph` 抽象 **Deferred / 视为弃用方向**，不以当前态阻塞其它模块。
- 验证基线为 **tests-required**：`cmake --build build --target MGF_Tests`，然后 `bin/MGF_Tests.exe`。

## 关联项目

| 项目 | 用途 |
| --- | --- |
| minEngine | 参考实现载体（类型系统、反射、序列化、AssetManager 等基础设施） |
| minCraft | minEngine 上的 gameplay 业务参考（itemDef & Instance、BlockDef 等） |
| min-agent-workflows | 本仓协作与治理文档的模板来源 |

三者均为**只读参考**：发现引擎能力缺口写在本仓，不在参考仓就地改代码。

## 不承诺的边界

- 不承诺可直接编译、开箱即用的成品框架；这里交付的是设计与契约，实现随需求逐步落地。
- 不承诺覆盖所有 gameplay 子系统；领域码按真实需求逐个追加，不做预置设计。
- 不承诺与任何特定引擎 API 绑定；迁移时允许、也预期需要重写代码。
- 不承诺文档冻结：设计会被评审推翻，推翻过程记录在 `docs/ai/PROGRESS_LOG.md`。

## 工程与构建

本节描述仓库根目录的 CMake 工程：它把 minEngine 作为 `ENGINE_ROOT` 拉进来，产出 game module DLL、
独立游戏宿主与编辑器宿主，并提供一个 doctest 测试可执行。

### 工具链要求

- CMake ≥ 3.20（实测 4.2.0-rc1）。
- **生成器必须与引擎树一致**：本仓使用 `MinGW Makefiles` + MinGW-Builds GCC 15.2.0
  （`D:/Dev/mingw64/bin/g++.exe`），与 `minEngine/minEngine/build/CMakeCache.txt` 里记录的
  `CMAKE_GENERATOR` / `CMAKE_CXX_COMPILER` 相同。不要混用 Visual Studio 或别家的 MinGW：
  引擎侧的 `libminEngine.dll` 与第三方 DLL 由同一套运行时构建。
- `ENGINE_ROOT` 指向 minEngine **仓库根**（不是 `minEngine/minEngine`），默认值已写为
  `D:/Dev/GitRepo/minEngine`，可覆盖。

### configure / build

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
```

只想验证接线、不想要测试目标时：追加 `-DMGF_BUILD_TESTS=OFF`。

打包（复制 exe + 运行时 DLL 到 `Staging/minGameplayFramework/`，不做 Cook）：

```powershell
cmake --build build --target package-minGameplayFramework
```

### 产物位置

所有运行期产物都写在仓库根的 `bin/`（`MINENGINE_OUTPUT_DIR`）：

| 产物 | 说明 |
| --- | --- |
| `bin/minGameplayFramework.exe` | 独立游戏宿主（`Source/Programs/GameMain.cpp`） |
| `bin/minGameplayFrameworkEditor.exe` | 编辑器宿主（引擎 `Editor/src/EditorMain.cpp` + 本模块） |
| `bin/minGameplayFramework.dll` | game module（反射注册、gameplay 代码） |
| `bin/libminEngine.dll`、`bin/glfw3.dll` 等 | 引擎与第三方运行时 DLL |
| `bin/MGF_Tests.exe` | doctest 测试可执行 |

`build/` 是 CMake 中间目录，`bin/` 与 `build/` 都已被 `.gitignore` 忽略。

### 测试

```powershell
ctest --test-dir build --output-on-failure
```

`MGF_BUILD_TESTS`（默认 `ON`）控制 `MGF_Tests`：`tests/Main.cpp` 提供 doctest 的 main，
用例放在 `tests/` 下（`tests/PlaceholderTestCases.hpp` 是链路自检）。CTest 通过 `add_test` 注册。

### EngineConfig 由 CMake 生成

configure 阶段会写 `bin/EngineConfig.meconfig`，其中 `EngineDefaultAssetsRoot` 指向引擎树的
`Assets/EngineDefault`（相对 `bin/` 的路径）。Dev 下两个宿主都以 cwd=`bin` 启动，PathRegistry
靠这个文件找到引擎内置资源（编辑器 Play 按钮图标等）。引擎树里缺少该目录时只发 `WARNING`，不失败，
但此时内置资源不会被解析。

### 反射 codegen

`Source/minGameplayFramework/` 是本项目的 gameplay 代码根，也是 codegen 的 `SRC_ROOT`；
带反射元数据的头文件必须放在这个目录（含子目录）。生成结果写到 `Generated/Reflection/`。

