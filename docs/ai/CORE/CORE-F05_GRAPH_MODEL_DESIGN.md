# CORE-F05 图模型设计
> 状态：Draft
> 作者：项目作者
> 最后更新：2026-10-26
> 关联契约：C1、C2、C3、C4、C5、C7、C8、C9、C11、C12、C13、C14、C17、C18、C20、C21、C22、C23、C24、C27、C28、C29、C30、C31、C33、C37、C38、C43、C44、C45、C46
> 上游特性：CORE-F01（分层与跨块契约）。依赖 CORE-F02（属性）、CORE-F03（内容定义与实例）、CORE-F06（会话与子系统所有权）。

## 1 问题与动机

技能树、科技树、任务图、对话图在多数项目里被实现成四套互不相干的东西：各自的结构体、各自的推进逻辑、各自的存档字段。开发者每次都要重写一遍「前置满足才可用」「完成才解锁下一格」「已经走过的对话节点不能再走」。更糟的是每套实现都顺手把派生状态写进存档，于是版本一改、属性一调，存档里就留下永远无法自证的历史结论。

本 spec 要回答的是同一个问题：**这四种业务里，「结构」与「语义」的边界画在哪里，才能让一份内核同时容纳它们，且不需要为每一种业务改内核。**

参考宿主的事实基线（CORE-F01 §1.1）给了三条硬边界：反射属性没有 inline struct 类别，因此图节点与 payload 只能是 Instanced 对象或扁平化字段；没有 GAS，因此「授予技能」这类后果必须由本仓自建并在明确的层落地；执行模型单线程同步、状态变更只在 Variable 相位，因此图推进是一个显式状态机而不是事件链。

本 spec 的结论可以一句话概括：**内核只拥有「节点集合 + 边集合 + 一张状态迁移表 + 一个只读访问器」，语义（什么算前置满足、单调还是可回退、激活之后发生什么）全部由按图种类注册的解析器承担；派生状态一律不落盘，落盘的只有决策与未完成的副作用事务。**

两条从外部经验里直接采纳的结论，本 spec 全文按它们成立：

1. 图文档只存结构。布局坐标、注释、颜色这类作者信息与语义结构分离，规则层永不读取它们。一旦让图文档同时承载运行期结论，内容与存档就互为镜像，两边都会漂。
2. 持久化只存「已决定的状态」与「挂起事务」。可由图结构 + 外部属性重算出来的东西不写盘。反例的腐坏过程在第 6 节展开。

## 2 目标与非目标

### 2.1 目标

- **G1 一份图文档结构契约。** 节点、边、图级元信息三段，字段类型与唯一性规则可被装载校验逐条检查。
- **G2 一套多态 payload 承载方式。** 在反射无 inline struct 的前提下给出唯一结论（内联 Instanced 子对象），并说明按 `nodeTypeId` 白名单实例化的完整路径。
- **G3 一个可被策略收窄的状态机。** 四状态与完整迁移表定义在内核，单调性与并发上限由解析器声明，内核不写任何业务特例。
- **G4 一组可插拔的校验策略。** 无环策略用 Kahn 并同时报告不可达节点；允许环策略不做拓扑排序，运行期只靠已访问集合与步数上限。运行期绝不做环检测。
- **G5 一种业务一个解析器。** 新增一种图业务只新增 payload 类型与一条注册项，不改内核、不改文档结构、不改存档格式。
- **G6 一条「派生状态不落盘」的落地路径。** 明确列出哪些是派生、装载后如何重算、如何用对账用例证明重算与逐帧推进等价。

### 2.2 非目标

- **不做图编辑器。** 第一版只在编辑器宿主里填写 DataAsset（与 CORE-F01 Q11 的建议一致）；没有节点画布、没有连线交互、没有自动布局。作者坐标由数据给出，缺省为零不影响任何判定。
- **不做对话推进的运行时脚本。** 不引入 DSL、不引入条件表达式求值引擎、不做文本标记语言。「这一句是否可说」由代码里的解析器与 payload 判定，不由数据里的字符串解释执行。
- **不做任务奖励结算。** 奖励是业务侧在观察到节点完成后自己做的事务；本 spec 只负责「节点进入了 Done」这个事实可被可靠观察，不负责发钱发物。
- **不做图的运行时热重载。** 内容包在加载闸口一次性装载（C23），运行期换图 = 新建实例。
- **不做图实例的跨会话迁移。** 与 C15 一致：只保证同一 schemaVersion 下的自洽往返。
- **不做多线程与并发推进。** 与 CORE-F01 的假设一致；图推进只在 Variable 相位、单线程。
- **不做寻路、不做图数据库查询、不做子图组合。** 图是内容的结构描述，不是通用图算法库。

## 3 术语

共享概念（ContentId、TagName、问题项与问题列表、失败三级分类、schemaVersion、RAII 订阅句柄、tick 相位规则、实例三段形态）一律按 CORE-F01 §3 与 §5 的定义使用，本文不另起定义。以下只列本文新增的概念。

| 术语 | 定义 | 归属层 |
| --- | --- | --- |
| 图种类 graphKind | 一个代码注册的判别串，表明这张图属于哪种业务（技能树 / 科技树 / 任务图 / 对话图）。一种图种类对应且只对应一个解析器注册项 | 核心（判别串值）/ 适配层（注册表） |
| 节点类型 nodeTypeId | 节点 payload 的类型判别串，代码注册，图种类注册项声明它允许哪些。内容里只出现这个串，不出现反射类名 | 核心（判别串值）/ 适配层（白名单与实例化） |
| payload | 挂在节点上、由该节点持有的多态子对象，承载这一种图对「这个节点是什么意思」的全部参数 | 适配层（反射派生）/ 核心（只读访问器接口） |
| 解析器 Resolver | 按图种类实现的业务对象：声明语义与白名单、判定入边是否满足、在迁移提交后执行副作用 | 适配层（实现）/ 核心（接口） |
| 语义声明 Semantics | 解析器给出的两件事：是否单调（是否允许回退迁移）、同时 Active 的节点上限 | 核心值类型 |
| 校验策略 ValidationPolicy | 结构校验算法与运行期步数上限的提供者；选项为无环策略与允许环策略 | 核心（接口与实现） |
| 已决定状态 | 由一次决策产生、装载后必须原样恢复的状态：只有 Active 与 Done | 核心 |
| 派生状态 | 由已决定状态 + 图结构 + 外部属性算出来的状态：Locked 与 Available 的区别 | 核心（可重算） |
| 挂起事务 PendingTransaction | 状态已提交、副作用尚未确认的一次迁移；必须落盘，装载后重试 | 核心 |
| 重挂 re-arm | 把 Done 节点退回 Available 以便再次经历，仅允许环语义（流语义）使用 | 核心 |
| 入边满足函数 | 解析器提供的一个判定：一个节点的所有入边在其前提上是否已满足（全满足 / 任一满足 / 业务自定义） | 核心（接口）/ 适配层（实现） |
| 一次遍历 pass | 从若干起点沿边推进的一轮；受步数上限约束，用于在允许环的图里防止自旋 | 核心 |

## 4 设计

### 4.1 图文档的结构与元信息

图文档分三段：图级元信息、节点集合、边集合。除此之外没有第四段。

| 段 | 字段 | 类型 | 说明 |
| --- | --- | --- | --- |
| 图级 | graphId | ContentId（kind 固定为 `graph`；资产侧值类型用 CORE-F03 的 `MGFContentIdRef`） | 图的稳定身份，装载期唯一（C1） |
| 图级 | graphKind | 代码注册判别串 | 决定解析器、payload 白名单、校验策略（C8） |
| 图级 | schemaVersion | 整型 | 图文档自身的语义版本（C12） |
| 节点 | nodeId | ASCII 串，图内唯一 | 作者给定、发布后不改（C1 的稳定性承诺在图的粒度上复用） |
| 节点 | nodeTypeId | 代码注册判别串 | 选择 payload 类型；装载期对白名单校验 |
| 节点 | payload | Instanced 子对象引用 | 见 4.2；一对一，其具体类型由 nodeTypeId 决定 |
| 节点 | authorX / authorY | 浮点 | 作者坐标，编辑器专用 |
| 节点 | authorNote | 字符串 | 作者标注，编辑器专用 |
| 边 | from / to | 两个 nodeId | 有向边；语义（前置、流程、选择）由 graphKind 定义 |

内容 id 的编码不归本文管：资产侧与文档里的内容引用一律是 CORE-F03 的 `MGFContentIdRef`（kind + name 的值类型）及其规范序列化形态，本文不再自定拼接格式、分隔符或包装结构；6.2 / 6.3 草图里的 `{ "kind": …, "name": … }` 只是该值的一种示意写法。

```cpp
// Sketch only; not the final signature.

/// @brief One node in a graph document. Carries authoring data and owns one payload object.
class MGFGraphNodeDef : public MEObject
{
public:
    /// @brief Stable identifier, unique inside its graph and never rewritten after publication.
    std::string m_NodeId;

    /// @brief Code-registered discriminator that selects the payload type for this node.
    std::string m_NodeTypeId;

    /// @brief Editor-only placement. No rule may read it.
    float m_AuthorX = 0.0f;
    float m_AuthorY = 0.0f;

    /// @brief Editor-only annotation. No rule may read it.
    std::string m_AuthorNote;

    /// @brief Payload owned by this node; the concrete type comes from the node type registry.
    MGFGraphNodePayload* m_Payload = nullptr;
};
```

三条结构性决定及理由：

1. **节点是 Instanced 对象，不是 ME_STRUCT。** C9 禁止把 struct 直接作为 DataAsset 的反射属性；节点要用「数组 + Instanced 对象」形态。备选的「多条平行 primitive 数组按下标对齐」（nodeIds[]、nodeTypeIds[]、payloads[]）被否：下标对齐在作者插入或重排一个节点后全错，且无法在反射层表达「这几条数组必须等长」，错误只能靠自写校验事后发现。
2. **边只存 (from, to)，不存条件、不存权重、不存标签。** 条件是业务语义，放进节点 payload 的附加判定里（4.2 的 `CheckExtra`），因为条件的数量与形态完全由业务决定，一旦进入结构层，内核就要理解条件表达式，解耦即失效。同一对节点之间的重复边是装载期错误。
3. **作者坐标与标注是元信息，规则层永不读取。** 这是「图文档只存结构」的落点：可检验的判据不是「别写注释」而是「图内核与解析器的代码里不存在读取这两个字段的路径」。装载器也不需要它们，只在编辑器宿主与差异比较里使用。

图文档里不出现任何运行期字段（状态、访问计数、当前进度）。运行期状态在同实例的一张独立状态表里（4.6），其持久化形态见第 6 节。

### 4.2 payload 的多态承载与实例化

#### 4.2.1 结论：payload 是节点内联持有的 Instanced 子对象

四个候选与被否的理由：

| 候选 | 结论 | 理由 |
| --- | --- | --- |
| 节点内联 Instanced 子对象（选中） | 采纳 | 符合 C9 允许的形态；payload 是「某个节点在本图里的参数」，没有独立身份，不需要独立装载 |
| payload 独立资产 + 资产引用 | 否决 | 违反 C2/C3：规则数据里出现资产引用，把装载顺序泄露进规则；且 payload 复用价值极低（同一份 payload 几乎不会出现在两张图里） |
| payload 用 ContentId 引用一份 payload 定义 | 否决 | nodeTypeId 已经表达了「是什么类型」，再引入一层 id 就是两处真值；且 payload 定义注册表会把加载期依赖再拉长一层，收益为零 |
| payload 直接内联 ME_STRUCT | 否决 | 违反 C9（反射无 inline struct 类别） |

所以序列化关系是**内联**：payload 由节点持有，随图资产同一份文档落盘，不产生独立资产、不产生独立 id。这也顺带解决了「payload 的运行期可变性」问题 —— 它根本不在运行期状态里（4.2.4）。

#### 4.2.2 nodeTypeId → 具体类型：白名单先校验，再实例化

内容里永远只出现 `nodeTypeId` 字符串，**不出现反射类名**。映射表在代码里：

```cpp
// Sketch only; not the final signature.

/// @brief One registered node type: the discriminator content writes plus the payload type it selects.
struct MGFNodeTypeRegistration
{
    std::string m_NodeTypeId;       ///< Discriminator written into graph content.
    std::string m_PayloadClassName; ///< Reflected class instantiated for this discriminator.
};
```

实例化路径固定为四步，顺序不可交换：

1. 反序列化节点骨架：读出 `nodeId`、`nodeTypeId`、作者字段。此时不创建 payload。
2. 校验：`nodeTypeId` 必须在该 `graphKind` 注册项声明的白名单里；不在，收集一条问题项，跳过实例化（继续校验其余节点，C18）。
3. 实例化：按注册项的类名走白名单化反射工厂创建 payload 对象，挂到节点上，再由反射反序列化填充它的字段。
4. 内容引用解析：遍历 payload 声明的 ContentId，进入 C13 的第二段解析。

这条顺序的意义是：**白名单校验发生在任何对象被创建之前**，因此「内容指定一个任意反射类」在物理上不可能发生（CORE-F01 §8「反射实例化工厂（按类名创建，带白名单）」）。若实现上反射反序列化必须在读节点时就把子对象创建出来（引擎机制差异），则第 2 步前移为一个独立的预扫描段，对整份文档的 nodeTypeId 做一次白名单核对；契约只约束「校验先于实例化」这个顺序，不约束具体切入点。

#### 4.2.3 payload 基类提供什么

只提供三个只读入口，一个写路径都没有：

```cpp
// Sketch only; not the final signature.

/// @brief Base for every payload type. Read-only by contract: payloads are content, not state.
class MGFGraphNodePayload : public MEObject
{
public:
    /// @brief Discriminator this payload answers to; must match the node type it was registered under.
    virtual const char* GetNodeTypeId() const = 0;

    /// @brief Extra gating beyond graph structure, evaluated in the variable phase.
    /// @warning Must be a pure query: no mutation, no content lookup side effects, no dispatch.
    virtual EPayloadCheck CheckExtra(const MGFNodeQueryContext& context) const
    {
        return EPayloadCheck::Allowed;
    }

    /// @brief Appends every content reference this payload carries, so the loader resolves them in one pass.
    virtual void CollectContentRefs(MGFContentRefSink& sink) const {}
};
```

为什么是这三个而不是更多：

- `GetNodeTypeId` 让白名单校验可以反查「这个对象是不是它自称的那一类」，防止注册表与实现漂移。
- `CheckExtra` 是「结构之外的门槛」的唯一入口：属性阈值、道具持有、任务已接，全部走这里，内核完全不知道它们存在。
- `CollectContentRefs` 是 C13 两段式解析在图上的落点：payload 指向的技能 / 科技 / 任务全是 ContentId，由装载器统一收集、统一解析、统一报问题。

#### 4.2.4 payload 与业务内容的指向关系

payload 里指向其它内容的字段一律是 ContentId（C2），例如技能树的 payload 声明「本节点授予哪个技能」、科技树的 payload 声明「本节点解锁哪项配方」、任务图的 payload 声明「本节点推进哪个任务步骤」。payload 不持有资产引用、不持有对象指针、不持有注册表句柄。

payload 是**内容**，不是状态（C4）：装载后只读，运行期不改写它的任何一个字段。业务需要「这个实例在这个节点上累计了几次」这类数据时，写实例的 tag 键增量表（C5），不写 payload。这条边界是可检查的：payload 类型的反射属性里不允许出现语义为 current / visited / count / progress 的字段。

### 4.3 节点状态机

#### 4.3.1 四个状态

```cpp
// Sketch only; not the final signature.

/// @brief Node state. Only Active and Done are persisted; Locked and Available are recomputed.
enum class EMGFNodeState : uint8_t
{
    Locked,     ///< Structural or extra gating not satisfied yet.
    Available,  ///< Gating satisfied, not yet taken; the only state an activation may start from.
    Active,     ///< Taken and currently in effect.
    Done        ///< Resolved and closed; never returns on its own.
};
```

语义要点：

- `Locked` 与 `Available` 的区别**完全**由「入边满足函数 + payload 的 CheckExtra」决定，不承载任何历史。它们是派生状态（4.6）。
- `Active` 是「这个节点正在生效」：技能树上意味着技能已授予且保持、任务图上意味着目标进行中、对话图上意味着这是当前一句。
- `Done` 是终态，且**不区分成功与失败**。业务如果需要「失败」这一维度，在进入 Done 时往实例增量表写一条 tag 键（细化 C5），内核不引入第五个状态。

#### 4.3.2 完整迁移表与策略收窄

内核定义全表，图种类通过语义声明收窄。内核里不存在 `if (graphKind == SkillTree)` 这样的分支。

| 源 | 目标 | 触发者 | 单调语义 | 流语义 | 钩子 |
| --- | --- | --- | --- | --- | --- |
| Locked | Available | 重算路径（入边与附加判定满足） | 允许 | 允许 | 无 |
| Available | Locked | 重算路径（前提不再满足） | **禁止** | 允许 | 无 |
| Available | Active | 激活请求 | 允许 | 允许 | OnCanActivate → OnActivated |
| Active | Available | 反激活请求 | **禁止** | 允许 | OnDeactivated（退出原因 = 回退） |
| Active | Done | 完成请求 | 允许 | 允许 | OnDeactivated（退出原因 = 完成） |
| Done | Available | 重挂请求 | **禁止** | 允许 | 无 |
| Locked | Active | — | 禁止 | 禁止 | — |
| 任意 | 任意 | 其它组合 | 禁止 | 禁止 | — |

三个结构性约束：

1. **外部只能请求四种动作**：Activate / Complete / Deactivate / Rearm。外部**不能**请求「把某节点设为 Available」—— Available 只能由重算路径产生。这条把「派生状态的唯一产生者」写死在类型上：请求类型的取值集合里没有 Available。
2. **单调语义下所有回退迁移被拒绝**，返回明确失败码（不是静默忽略、不是断言崩溃）。这就是技能树「激活后永久生效」在契约里的写法：它不是内核的一个特例，而是**语义声明 `m_bMonotonic = true` 对迁移表的收窄**。可检验的表述是「对单调图实例请求 Deactivate 或 Rearm，返回 `E_TransitionRejectedBySemantics`，状态不变」。
3. **同一内核容纳对话图的环**：对话图声明 `m_bMonotonic = false`，于是 Available→Locked、Active→Available、Done→Available 全部可用，节点可以被重挂并再次经历；环的风险由「校验策略是允许环策略 + 运行期已访问集合 + 步数上限」承担（4.4），而不是由单调性承担。

因此「同一内核容纳两种语义」的答案是分层的：**内核给状态与迁移，语义给允许的迁移子集，校验策略给环的处理方式，解析器给入边与副作用。** 四者都在代码注册项里，没有一条在数据里，也没有一条在 `if` 里。

#### 4.3.3 并发 Active 上限

技能树允许多个技能同时 Active；对话图只允许一个当前节点。这是语义差异，写进语义声明：

```cpp
// Sketch only; not the final signature.

/// @brief Transitions a graph kind admits, plus its concurrency limit.
struct MGFGraphSemantics
{
    bool m_bMonotonic = true;           ///< When true every backward transition is rejected.
    uint32_t m_MaxConcurrentActive = 0; ///< Zero means unbounded.
};
```

超限的激活请求返回明确失败码，且不改变任何状态。上限是「语义」而不是「配置」：它由注册项给出，资产里没有这个字段（否则内容可以自己放宽不变量的守卫）。

#### 4.3.4 反激活：第一版提供，作用域是单节点

决定：**提供**，但仅在流语义下可用；单调图里调用它是调用方错误（调试期断言 + 发布期失败码，符合 C17 的运行期断言类）。

后继状态一致性不靠级联，靠三条规则：

1. **内核不自动级联。** 一次反激活只写一条迁移记录（外加派生重算，派生重算不写「决策」）。哪些后继一起退，由解析器决定并显式发出同一批请求。
2. **批是事务。** 一次提交可以带多个请求；批内顺序由调用方（解析器）给出，内核只做去重与合法性校验，**绝不做排序** —— 排序意味着运行期拓扑排序，在允许环的图里根本不成立（见 5.3 的 K19）。批提交后统一重算派生状态、统一广播通知，外部观察不到中间态。
3. **Done 不回退。** 已完成的节点不会被反激活连坐。业务若确实要「撤掉一个已完成的科技」，那是新建实例或业务侧自己补偿的问题，不是内核把终态变回非终态。这条与单调性无关，是无环语义也要守的底线：Done 回退会让「曾经授予过的东西」与「图里说没授予」长期矛盾。

#### 4.3.5 钩子的参数与调用时机

三个钩子定义在解析器上（不是 payload 上）：

```cpp
// Sketch only; not the final signature.

/// @brief Read-only view a resolver gets during a transition; it cannot mutate the state table.
class MGFNodeTransitionRecord
{
public:
    const char* GetGraphKind() const;             ///< Registered discriminator of the graph.
    MGFInstanceId GetInstanceId() const;          ///< Owning graph instance.
    const std::string& GetNodeId() const;
    const std::string& GetNodeTypeId() const;
    const MGFGraphNodePayload* GetPayload() const; ///< Never null once the document passed the load gate.
    EMGFNodeState GetFromState() const;
    EMGFNodeState GetToState() const;
    uint32_t GetAttempt() const;                  ///< Retry count of the pending transaction, starting at one.
};
```

| 钩子 | 时机 | 返回 | 约束 |
| --- | --- | --- | --- |
| `OnCanActivate` | Available→Active 提交**之前**，在 Variable 相位 | 允许 / 拒绝（原因串） | 纯查询：不得写状态、不得施加效果、不得派发事件 |
| `OnActivated` | Available→Active 提交**之后**，同一事务内 | 成功 / 问题项列表 | 副作用在这里；必须按 (实例, 节点, 目标状态) 幂等 |
| `OnDeactivated` | 离开 Active（Active→Available 或 Active→Done）提交**之后** | 成功 / 问题项列表 | 携带退出原因（回退 / 完成）以便区分处理 |

钩子出错的处理，按 C17 分级：

- `OnCanActivate` 返回拒绝 = **正常业务结果**，不是错误：迁移不发生，状态不变，返回 `Denied`（C21 的结构化结果），不生成问题项，不写日志噪音。
- `OnActivated` / `OnDeactivated` 返回失败 = **副作用没做成**。此时状态已经提交，不能回滚（回滚等于要求业务提供逆运算，与 C6「不做逆运算」的精神冲突），处理方式是：记一条问题项（定位到实例、节点、钩子）、把这次迁移登记为挂起事务、在后续 Variable 相位有界重试（默认上限 3 次）。重试用尽仍失败则返回失败结果码并保留挂起事务，等待下一次装载重试。
- 钩子内抛出异常（若实现语言有异常）与返回失败同路：捕获、转成问题项、按上一条处理。钩子不得让异常穿出内核。

#### 4.3.6 挂起事务

挂起事务是唯一允许存在「状态已提交、副作用未确认」的形态，形如 `(nodeId, fromState, toState, exitKind, attempts)`。它必须落盘（第 6 节），因为不落盘的后果是：装载后状态说「技能已授予」，而技能实际上从未被授予，再也没有人知道这件事。

装载时的处理顺序是强制的：先恢复状态表 → 再把挂起事务按落盘顺序重试 → 再跑派生重算。**已确认的迁移不重放钩子** —— 装载不是「把历史演一遍」，否则授予技能会被执行第二次。

### 4.4 有效性校验策略

#### 4.4.1 策略接口

```cpp
// Sketch only; not the final signature.

/// @brief Structural validation plus the runtime step bound for one graph kind.
class MGFGraphValidationPolicy
{
public:
    virtual ~MGFGraphValidationPolicy() = default;

    /// @brief Validates the whole document and appends every problem it finds; never stops at the first.
    virtual void Validate(const MGFGraphDoc& document, MGFIssueList& issues) const = 0;

    /// @brief Upper bound on node visits in one traversal pass of this graph kind.
    virtual uint32_t GetMaxStepsPerPass() const = 0;
};
```

注意接口里**没有** `DetectCycle`、`TopologicalSort`、`IsAcyclic` 这类方法：运行期拿不到环检测能力，这是刻意的（K28）。

#### 4.4.2 无环策略（Kahn）

`Validate` 的实现步骤：

1. 结构与引用完整性（id 唯一、边两端存在、无重复边、无自环）。
2. Kahn 拓扑剥离：反复移除入度为 0 的节点及其出边。
3. 剥离结束后仍有剩余节点 → 每个剩余节点都是环的成员或被环支配者；**逐个报告**剩余节点 id（C18 要求完整列表，不是只报「有环」）。
4. 可达性：从入口节点集合（图种类声明的入口；未声明时取入度为 0 的节点）正向遍历，未被访问到的节点逐个报告为「不可达」。不可达的级别是允许的降级（第 7 节），因为它是内容完整性问题而不是一致性问题。
5. 顶点数上限与边数上限（防止一份畸形资产把装载拖死）。

两个边界情况写清楚：自环在无环策略里是「长度为 1 的环」，报环而不是报另一类错误；可达性按**原图**算，环单独报，避免「环让所有下游都不可达」掩盖真实问题。

#### 4.4.3 允许环策略

不做拓扑排序，因为不存在。只做：

1. 结构与引用完整性（同上）。
2. 自环允许、平行边仍禁止。理由：自环是对话图里「再说一遍」的自然写法，平行边则是无意义冗余，两种语义不同。
3. 运行期 `GetMaxStepsPerPass()`：一次遍历 pass 内访问节点数的上限。遍历时维护一个 pass 内已访问集合，再次访问同一节点即判定为「本 pass 内的重访」并结束 pass（返回 `StepCapReached` 或 `RevisitDetected`），**不**去判定全图是否有环、也**不**去判定这是不是一个真环。已访问集合按 pass 建立，pass 结束即丢弃，不落盘。这一点很重要：已访问集合若落盘，它就是派生状态，腐坏方式与第 6 节的反例完全相同。

#### 4.4.4 作者期 / 加载期 / 运行期的分工

| 时刻 | 做什么 | 不做什么 |
| --- | --- | --- |
| 作者期（编辑器宿主保存时） | 完整策略校验 + 白名单 + ContentId 存在性；问题直接呈现给作者 | 不写运行期状态 |
| 加载期（加载闸口） | 同一套 `Validate` 完整跑一遍 + payload 内容引用两段式解析（C13） | 不信任作者期结果（资产可能被外部改动） |
| 运行期（Variable 相位） | 只做不变量断言与步数上限 | **绝不做环检测、绝不做拓扑排序、绝不重跑 Validate** |

运行期不做环检测的理由有两条，都是硬的：一是 C23「tick 里不做重活」，一份千节点的图在 tick 里做 Kahn 是不可接受的；二是允许环的图里「有没有环」这个问题本身没有业务含义，问它只会产出一个没人知道该怎么处理的布尔值。

#### 4.4.5 策略选择绑在哪一层

绑在**解析器注册项**上，与白名单、语义声明放在同一个结构里，在组合根里一次性注册。

```cpp
// Sketch only; not the final signature.

/// @brief One composition-root entry: everything that makes a graph kind executable.
struct MGFGraphKindRegistration
{
    std::string m_GraphKind;                            ///< Discriminator written into graph content.
    MGFGraphSemantics m_Semantics;                      ///< Monotonicity and concurrent active limit.
    std::vector<MGFNodeTypeRegistration> m_NodeTypes;   ///< Payload whitelist.
    const MGFGraphValidationPolicy* m_ValidationPolicy; ///< Structural validation and step bound.
    MGFGraphResolver* m_Resolver;                       ///< Business implementation.
};
```

不绑在图资产上的理由：如果资产能声明自己的校验策略，一份坏图只要写上「我是允许环策略」就绕过了无环校验，验证约束变成可选建议。校验策略是语义，语义必须取得代码契约身份（C8），和 fragment 种类、问题种类同级。可检验的表述是：图资产的反射属性表里不存在任何选择校验策略或语义的字段。

### 4.5 解析器与业务解耦

#### 4.5.1 接口

```cpp
// Sketch only; not the final signature.

/// @brief Business implementation for one graph kind. The kernel never names a concrete resolver.
class MGFGraphResolver
{
public:
    virtual ~MGFGraphResolver() = default;

    /// @brief Discriminator this resolver answers to; must match its registration entry.
    virtual const char* GetGraphKind() const = 0;

    /// @brief Seeds the state table of a fresh instance and appends problems it finds.
    virtual void InitializeInstance(const MGFGraphDoc& document,
                                    MGFGraphStateTable& stateTable,
                                    MGFIssueList& issues) const = 0;

    /// @brief Decides whether the incoming edges of one node are satisfied right now.
    virtual MGFEdgeSatisfaction EvaluateIncoming(const MGFNodeEvalContext& context) const = 0;

    /// @brief Final gate before a node becomes active; a pure query with no side effects.
    virtual MGFActivationDecision OnCanActivate(const MGFNodeTransitionRecord& record) const = 0;

    /// @brief Side effects of a committed activation. Must be idempotent per attempt.
    virtual MGFTransitionResult OnActivated(const MGFNodeTransitionRecord& record) = 0;

    /// @brief Side effects of leaving the active state, for both revert and completion.
    virtual MGFTransitionResult OnDeactivated(const MGFNodeTransitionRecord& record) = 0;
};
```

`MGFNodeEvalContext` 只提供只读访问：图文档（const）、状态表（const 视图）、实例的增量表（const 视图）、以及一个只读的属性查询接口（9.1）。解析器**没有**直接改状态表的入口 —— 它只能通过内核的迁移入口提交请求（C29 单写入点、C46 依赖方向）。

#### 4.5.2 解耦的具体边界

「图不知道内容是什么」这句话要落成三条可检查的边界：

1. 内核（`mgf-core` 里的图部分）的类型集合里没有技能、科技、任务、对话这些词，也没有属性、背包。它只有：节点、边、状态、请求、问题项、策略、解析器接口。
2. 内核从不调用具体解析器，只经 `MGFGraphResolver&` 调用；依赖由组合根注入（C46）。
3. 内容侧的 payload 类型与解析器一起构成业务层；业务层可以自由知道技能、属性、背包，这是它的职责，不构成对内核的解耦破坏。**解耦的对象是内核，不是「业务内部不许分工」**。

#### 4.5.3 两种业务的差异表（说明「共享接口、语义不同」）

| 维度 | 技能树 | 对话图 |
| --- | --- | --- |
| 语义声明 | 单调（`m_bMonotonic = true`），同时 Active 不限 | 流（`false`），同时 Active = 1 |
| 校验策略 | 无环（Kahn） | 允许环（已访问集合 + 步数上限） |
| 入边满足 | 全部满足（All-of）：所有前置都 Done | 任一满足（Any-of）：任一条入边的来源已 Done 即可进入 |
| 激活触发者 | 玩家请求 + 前置满足 | 解析器在上一句完成后按选项推进一步 |
| Active 的含义 | 技能已授予并保持 | 这是当前显示的一句 |
| 离开 Active | 不允许（永久生效） | 允许（回到上一句 / 完成本句） |
| 反激活 / 重挂 | 被语义拒绝 | 正常路径 |
| 完成后的后果 | 解锁后继节点 | 由业务推进到下一句；不结算奖励 |
| payload 承载 | 授予的技能 ContentId、前置属性阈值 | 说话者、文本键、选项列表 |
| 状态文档 | 只存 Active / Done | 同左 |

同一套接口、同一套状态机、同一份存档格式，差异全部落在注册项与解析器实现里。这就是「一种图对应一种解析业务」的可操作含义：新增「配方解锁图」的做法是新增一个 payload 类型 + 一个解析器 + 一条注册项，diff 里不出现 `mgf-core`。

### 4.6 派生状态与持久化

#### 4.6.1 哪些是派生

| 数据 | 分类 | 理由 |
| --- | --- | --- |
| nodeId → Active / Done | 已决定 | 一次决策的产物，无法从图结构推出 |
| nodeId → Locked / Available | 派生 | 由已决定状态 + 入边满足 + payload 附加判定 + 外部属性算出 |
| 挂起事务 | 已决定（未完成） | 副作用是否做成这件事无法重算 |
| 入边满足缓存 | 派生 | 重算即可 |
| pass 内已访问集合 | 派生，且不跨相位 | 只在一次遍历内有效 |
| payload 全部字段 | 内容，不是状态 | 属于图资产，不进入实例状态文档 |
| 图实例的 tag 增量（如节点访问次数） | 已决定 | 走实例增量表（C5），不进入图状态段 |

#### 4.6.2 装载后的重算顺序

1. 反序列化状态文档，得到身份、已决定状态集合、挂起事务。
2. 状态表初始化：已决定状态原样恢复；其余节点置 `Locked`。
3. 重试挂起事务（有界）。**不重放已确认事务的钩子。**
4. 派生重算：对每个 `Locked` 节点跑入边满足函数 + `CheckExtra`，满足者提升为 `Available`（这一步只写内存状态表，不写「已决定」集合，不触发钩子）。
5. 一致性对账（仅测试配置）：从空状态重放同一输入，与第 4 步结果比对。

重算必须幂等：连跑两次的结果与跑一次相同，且不产生任何副作用、不派发任何事件、不触发钩子。这条不是为了优雅，而是为了「装载后重算」与「运行期逐帧推进到同一输入」能等价 —— 后者正是 CORE-F01 §9 Wave3 的第 2 条验收判据。

#### 4.6.3 反例：把派生状态写盘必然腐坏

腐坏过程：技能树节点 B 的前置是「力量 ≥ 10」。作者期玩家力量 12，B 被算成 Available 并写入存档。之后玩家卸下装备，力量掉到 8。装载时存档说 B 是 Available，图结构说 B 的前置不满足，两份真值冲突；任何一方胜出都会产生一种无法自证的错误状态：读存档就丢了属性，重算就丢了「当时是谁把它算出来的」。

第二种腐坏：Available 被写盘后，属性阈值被内容更新调高。老存档里一批本该 Locked 的节点是 Available，且**没有任何代码路径会重新检查它们**，因为它们已经是「已决定」—— 腐坏静默，直到玩家点了一个不该能点的节点。

发现方式就是可检验的形式：装载后重算必须与「从零推进到同一输入」一致；状态文档里不允许出现 Available / Locked 字面值。

### 4.7 时序与边界情况汇总

一次激活在 Variable 相位内的顺序（不可交换）：

1. 收请求（Activate + 目标节点）→ 校验状态是 Available、并发上限未超、语义允许该迁移。
2. `OnCanActivate`（纯查询）。拒绝则整体结束，状态不变。
3. 提交状态（Available→Active），写「已决定」集合，生成挂起事务。
4. `OnActivated`。成功则清除挂起事务；失败则保留并记问题项。
5. 派生重算（受影响节点）。
6. 提交后广播一次「图状态已变更」（通道 tag，无载荷，C24），本帧内同实例同通道去抖（C28）。

边界情况：

- 同一帧内对同一节点请求两次激活：第二次在步骤 1 被拒（状态已不是 Available），首次的结果不受影响。
- 批内包含「A 反激活」与「A 后继 B 反激活」：由解析器给出顺序（B 在前），内核只校验不排序。
- 步数上限触发：当前 pass 立即结束，返回 `StepCapReached`，已提交的迁移不回滚。
- 装载时状态文档引用了已从内容中删除的 nodeId：fail-fast，报出该 nodeId（这是内容变更与存档不一致，不能静默丢弃，否则丢的是玩家的进度）。
- 图种类未注册：fail-fast，报出 graphKind 与图的 ContentId。

## 5 契约

每条都是「规则 + 可检验」。标注「细化 Cn」表示它把总纲的那条契约落到图这一块，只补充总纲未覆盖的部分。

### 5.1 图文档结构

**K1 图文档只有结构三段。** 图文档 = 图级元信息（graphId、graphKind、schemaVersion）+ 节点集合 + 边集合；不出现任何运行期字段（状态、计数、进度、时间戳）。
细化 C11。
可检验：图文档的反射属性表里不存在语义为 state / current / visited / progress 的字段；运行期状态文档里不存在 nodes / edges 字段。

**K2 graphId 是 ContentId，kind 段固定为 graph。** graphKind 是另一个字段，不是 ContentId 的 kind 段；两者不得互相推导。
细化 C1。
可检验：装载期对重复 graphId 报问题项；`kind = graph, name = Skill.Warrior` 与 `kind = skill, name = Warrior` 是两回事，用例验证互不冲突。

**K3 nodeId 是图内唯一、发布后不改的 ASCII 串。** 非法字符集与 C1 对 name 的约束一致。
细化 C1。
可检验：重复 nodeId 报一条问题项并给出重复值；非法字符报一条并给出字符位置；两条同时存在时返回两条（C18）。

**K4 边只存 (from, to)。** 不存条件、权重、标签；同一对节点之间的重复边是装载期错误；自环是否合法由校验策略决定。
细化 C2。
可检验：边的反射属性只有两个 nodeId 字段；构造重复边，装载失败并给出该对 nodeId。

**K5 作者字段不参与任何语义。** 图内核、校验策略与解析器都不得读取 authorX / authorY / authorNote。
细化 C7。
可检验：在 `mgf-core` 与解析器源码上 grep 这三个字段名，结果为空；把某节点的坐标改成任意值，全部图用例结果不变。

**K6 图实例的额外状态走实例增量表，不新增图文档字段。** 需要「访问过几次」「哪个分支被选过」时，写实例的 tag 键增量（C5）。
细化 C5。
可检验：不改图文档反射属性、不改内核代码，仅通过增加一条 tag 键与一段解析器逻辑表达「同一节点第二次进入时走另一分支」，全部用例通过。

### 5.2 payload

**K7 payload 由节点内联持有，不是独立资产、不是 ContentId 引用、不是 ME_STRUCT。**
细化 C9（含总纲 Q7 建议 a 的落点）。
可检验：图文档的节点属性是 Instanced 对象引用；全仓 grep 资产句柄类型出现在图相关结构体上结果为空；header-tool 生成代码里图相关 struct 属性数为零。

**K8 内容里只出现 nodeTypeId，不出现反射类名。** nodeTypeId → 类名的映射只来自代码注册项。
细化 C8 / C9。
可检验：在示例内容资产里全文搜索 payload 类名字符串，结果为空；把内容里的 nodeTypeId 改成另一个已注册值，装载成功且生成对应类型的 payload。

**K9 白名单校验先于实例化。** nodeTypeId 必须在 graphKind 注册项声明的白名单里，否则收集问题项并跳过该节点的实例化。
细化 C9 / C17。
可检验：把 nodeTypeId 换成不在白名单里的值，装载失败且问题项定位到该 nodeId；同一份内容里另有一个合法节点，其问题项为零（证明校验是逐条收集而不是首错中断）。

**K10 payload 基类只有只读入口。** `GetNodeTypeId` / `CheckExtra` / `CollectContentRefs` 三个，没有写路径，没有对状态表的引用。
细化 C4 / C29。
可检验：payload 基类的公开成员函数集合逐条审查为无写语义；把 `CheckExtra` 换成「试图修改状态表」的实现时编译失败（接口不提供该入口）。

**K11 payload 的内容引用一律 ContentId，并进入 C13 第二段解析。**
细化 C2 / C13。
可检验：payload 类字段类型里不出现资产类型；构造一份 payload 引用两个不存在 id 的文档，装载返回包含这两个 id 的问题列表。

**K12 payload 是内容只读。** 运行期不得改写 payload 任何字段；per-instance 数据走实例增量表。
细化 C4。
可检验：「装载 → 反复推进图 → 读回 payload」逐字段与装载时一致；payload 属性表里没有 current / visited / count / progress 语义的字段。

**K13 payload 缺失或类型不在白名单是加载期 fail-fast。**
细化 C17。
可检验：把某节点的 payload 置空，装载失败并给出该 nodeId 与「payload 缺失」种类。

### 5.3 状态机与钩子

**K14 四状态与 4.3.2 的迁移表是唯一合法集合。** 表外组合一律拒绝。
细化 C29。
可检验：对每个 (源, 目标) 组合各一条用例，合法组合成功、非法组合返回 `E_IllegalTransition` 且状态不变。

**K15 单调性由语义声明收窄迁移表，内核无业务特例。**
细化 C8。
可检验：`mgf-core` 图相关源码里 grep 具体 graphKind 判别串结果为空；把同一份图文档以 `m_bMonotonic = true` 与 `false` 注册两次，回退迁移分别被拒与成功。

**K16 单调语义下的回退请求返回明确失败码，且状态不变。**
细化 C17 / C21。
可检验：对单调实例请求 Deactivate 与 Rearm，各返回一次失败结果码；调试配置下同时触发断言；断言关闭配置下仍返回同一失败码（C20）。

**K17 Done 在任何语义下都不能通过迁移路径离开。** 只有流语义的 Rearm 能把 Done 退回 Available；单调语义没有 Rearm。
细化 C11。
可检验：对 Done 节点请求 Activate / Deactivate 均失败；流语义实例 Rearm 后状态为 Available 且原 Done 记录被更新而不是新增。

**K18 外部只能请求 Activate / Complete / Deactivate / Rearm 四种动作。** Available 只能由重算路径产生。
细化 C29。
可检验：迁移入口参数类型的取值集合与上列四项一致（审查项）；不存在「设置状态」的公开入口。

**K19 反激活第一版提供，作用域是单节点，内核不自动级联。** 批内顺序由调用方给出，内核只校验不排序。
细化 C22 / C29。
可检验：一次反激活只产生一条状态迁移记录（派生重算不计）；传入乱序批时按传入顺序应用；重复 nodeId 的批被拒。

**K20 并发 Active 上限由语义声明，超限请求失败且不改变状态。**
细化 C8。
可检验：对话图（上限 1）在已有 Active 节点时再激活一个，返回 `E_ActiveLimitExceeded`；技能树同场景成功。

**K21 `OnCanActivate` 是纯查询，时机在提交之前。** 拒绝是正常业务结果，不生成问题项、不改状态。
细化 C21。
可检验：注入一个「总是拒绝」的解析器，激活返回 Denied、状态不变、问题列表为空；注入一个「在 CanActivate 里改状态」的解析器，调试期断言触发。

**K22 `OnActivated` / `OnDeactivated` 在提交之后、同一事务内执行，且必须幂等。**
细化 C29。
可检验：钩子实现里按 (实例, 节点, 目标状态) 记录调用次数，重试场景下副作用净效果只发生一次。

**K23 钩子失败 = 问题项 + 挂起事务 + 有界重试。** 状态不回滚。
细化 C17 / C18。
可检验：让 `OnActivated` 前两次失败第三次成功，第三次之后挂起事务清除且问题项为两条；一直失败到上限后返回失败码，挂起事务仍在。

**K24 挂起事务必须落盘，装载后先重试，而已确认迁移不重放钩子。**
细化 C11。
可检验：构造一份带挂起事务的状态文档，装载后钩子被调用且仅被调用一次；另构造一份不带挂起事务的文档，装载后钩子调用次数为零。

### 5.4 校验

**K25 校验策略与语义声明绑在代码注册项上，资产里没有选择字段。**
细化 C8。
可检验：图资产的反射属性表里没有策略名或语义字段；把一份有环的图声明为某个 graphKind，装载失败（证明内容不能自选策略）。

**K26 无环策略用 Kahn，在报环的同时逐个报告剩余节点，并单独报告不可达节点。**
细化 C17 / C18。
可检验：一份含两处环（一个长度 2、一个长度 3）的夹具返回五个 nodeId 的环问题项；一份含一个孤立节点的夹具返回不可达问题项且不含环问题项。

**K27 允许环策略不做拓扑排序；运行期用 pass 内已访问集合 + 步数上限。**
细化 C23。
可检验：允许环实例连续推进远超节点数目的步数后仍返回正常结果且不崩；超过上限返回 `StepCapReached`；已访问集合在 pass 结束后被清空（审查项：不落盘）。

**K28 运行期绝不做环检测、绝不做拓扑排序、绝不重跑全量校验。**
细化 C23。
可检验：`mgf-core` 的迁移与推进路径上不存在调用 `Validate` 的边（符号引用审查 + 一条计数器用例：一帧推进期间 Validate 调用次数为 0）。

**K29 结构校验只在加载闸口与作者期执行，运行期只做不变量断言。**
细化 C23 / C17。
可检验：在 tick 中触发一次图迁移，资产装载计数与校验计数均为零（C23 的计数器用例扩展到图）。

**K30 不可达节点的默认级别是允许的降级。** 记录警告并继续装载；开关关闭后升为 fail-fast。
细化 C17。
可检验：默认配置装载含孤立节点的图成功且产生一条警告；开关关闭后同一图装载失败。

**K31 悬空边恒为 fail-fast，本 spec 不引入「可选边」。** 与 C17 的澄清同口径：已声明的图边缺失恒为 fail-fast，不属降级；C17 表里「可选边目标缺失（丢弃该边）」只适用于非结构性引用，在图上不适用。
细化 C17。
可检验：边指向不存在的 nodeId，装载失败；配置里不存在让悬空边通过的开关。

### 5.5 解析器

**K32 一个 graphKind 对应且只对应一个解析器注册项。** 新增一种图业务 = 新增 payload 类型 + 解析器 + 一条注册项。
细化 C8 / C46。
可检验：重复 graphKind 的注册在启动期被拒；新增一个 graphKind 的 diff 里不含 `mgf-core` 目录下的文件。

**K33 解析器只能只读访问图状态，写只能经内核迁移入口。**
细化 C29 / C46。
可检验：`MGFNodeTransitionRecord` 与 `MGFNodeEvalContext` 的公开成员函数集里没有写语义入口；解析器绕过迁移入口直接改状态的写法编译失败。

**K34 入边满足函数属于解析器，内核只调用它。** 内核不内置 AND/OR 语义。
细化 C8。
可检验：同一份图文档配 All-of 解析器时仅当全部前置 Done 才 Available，配 Any-of 解析器时任一前置 Done 即 Available；内核源码里不存在「全部前置」这类判定。

**K35 激活到「授予技能」这类后果的映射声明在 payload、执行在解析器，内核只发出状态迁移事实。**
细化 C2 / C29。
可检验：把 payload 里声明的技能 ContentId 换掉，技能授予结果随之变化而内核代码零改动；解析器与 payload 之外（含内核）grep 技能相关类型名结果为空。

**K36 解析器产生的副作用必须在提交后执行，且失败不静默。**
细化 C21。
可检验：解析器在提交前施加效果的实现被用例检出（状态变更时序断言）；副作用失败产生问题项并按挂起事务重试。

### 5.6 派生状态与持久化

**K37 持久化只存：身份、graphId、schemaVersion、已决定状态（Active / Done）、挂起事务。**
依据 C11 的归位：图文档的决策状态（Active / Done）与挂起事务属允许落盘的决策数据，Locked / Available 不落盘、加载后重算（总纲 R3 的 B2 已把这句写进 C11 正文）。本文不另立「第四块数据」之类的分类。
细化 C11 / C12。
可检验：状态文档里不存在 Available / Locked 字面值；把 schemaVersion 改成 0 或 2，装载拒绝并说明版本（C12）。

**K38 条目存在性表达「已决定」，不用哨兵值。** 状态表里没有的节点 = 从未被决定；Locked / Available 根本没有条目可言，因为它们是重算出来的投影而不是被决定的决策（C11 的归位）。
细化 C14。
可检验：写入一条 Done 后往返，条目数不变；未决定的节点不出现条目，而不是出现值为 Locked 的条目。

**K39 装载后派生重算必须与逐帧推进到同一输入的结果一致，且重算幂等、无副作用。**
细化 C11。
可检验：对账用例 —— 同一实例「装载后重算」与「从空状态逐帧推进」得到的状态表逐节点相等；连续重算两次结果相同且钩子调用次数为零。

**K40 状态文档引用未知 nodeId 是 fail-fast。**
细化 C17。
可检验：构造一份引用已删除 nodeId 的状态文档，装载失败并给出该 nodeId。

**K41 状态迁移与派生重算只在 Variable 相位执行。**
细化 C22。
可检验：在 Fixed 相位调用迁移入口，调试期断言并返回失败（与 C22 的通用断言同一条实现）。

### 5.7 所有权

**K42 图实例由持有者子系统管理，框架不提供图管理器单例。** 选哪个子系统由使用方按 C30 的生命周期判据决定。
细化 C30 / C31。
可检验：全仓不存在图相关的进程级单例注册表；两个 GI 实例并存时各自的图状态互不可见（C32 的图侧用例）。

## 6 数据与序列化

### 6.1 三种数据，三种生命周期

| 数据 | 载体 | 何时写 | 是否落盘 |
| --- | --- | --- | --- |
| 图文档（结构 + payload） | DataAsset（内容资产） | 作者期在编辑器宿主里 | 是，作为内容落盘；运行期只读 |
| 图状态文档 | JSON（实例快照的一段） | 显式保存请求（C16） | 是 |
| pass 内已访问集合、入边满足缓存 | 内存 | 重算与遍历期间 | 否 |

### 6.2 图文档的规范序列化形态

图文档由反射驱动读写，但输出顺序是规范化的，这样「往返字节等价」才是一个有意义的判据：节点按 nodeId 字典序输出，边按 (from, to) 字典序输出，payload 字段按反射声明顺序输出。

```json
{
  "schemaVersion": 1,
  "graphId": { "kind": "graph", "name": "Skill.Warrior" },
  "graphKind": "skill_tree",
  "nodes": [
    {
      "nodeId": "node.charge",
      "nodeTypeId": "skill.node",
      "authorX": 120.0,
      "authorY": 64.0,
      "authorNote": "starter",
      "payload": {
        "grantedSkill": { "kind": "ability", "name": "Charge" },
        "requiredStrength": 10
      }
    }
  ],
  "edges": [ { "from": "node.charge", "to": "node.cleave" } ]
}
```

字段名以最终的反射声明为准，但层次与顺序约束是契约。

### 6.3 图状态文档

```json
{
  "schemaVersion": 1,
  "ownerRef": "player.0001",
  "graphId": { "kind": "graph", "name": "Skill.Warrior" },
  "states": [
    { "nodeId": "node.charge", "state": "Active" },
    { "nodeId": "node.cleave", "state": "Done" }
  ],
  "pending": [
    {
      "nodeId": "node.cleave",
      "from": "Active",
      "to": "Done",
      "exitKind": "Completed",
      "attempts": 1
    }
  ]
}
```

字段约束：

- `states` 里只允许 Active 与 Done（K37）；按 nodeId 字典序输出。
- `pending` 按 (nodeId, toState) 字典序输出，`attempts` 是重试次数而不是总数。
- 不出现 Locked / Available，不出现「所有节点」的完整列表 —— 缺条目即未决定（K38）。
- 不出现资产路径、对象指针、tag 容器（C3 / C40）。payload 的全部内容不在此文档内（它是内容，不是状态）。

### 6.4 往返等价如何保证

- 写：状态表按 nodeId 字典序导出；省略未决定的节点。
- 读：只恢复出现过的条目；其余节点置 Locked，再走派生重算。
- 等价判据：save → load → save 两次字节序列相等（C11）。这条会自动排除「读进来补了一堆默认条目」的实现错误。
- 版本政策按 C12 的区分落地：内容资产的版本字段只记录、不分支；持久化文档的 `schemaVersion` 必须分支，缺失或未知即拒绝，两者不可互相套用。图文档与图状态文档都是本框架自己序列化 / 反序列化的文档，属后者；payload 在文档里的承载不是「资产版本字段」，因此同样走文档的分支。当前只有版本 1 一条分支，其它值拒绝并报问题项（C12）。
- payload 字段演进不引入独立版本号：payload 字段变化等价于图文档 schema 变化，随图文档的 `schemaVersion` 一起推进，不写空迁移函数。

### 6.5 与实例快照的关系

图实例按 C5 的三段形态存在（定义引用 + tag 键增量 + 显式覆盖列表）。图状态段不是第四类数据：按 C11 的归位，图文档的决策状态（Active / Done）与挂起事务属**允许落盘的决策数据**，Locked / Available 不落盘、加载后重算；总纲 §3 术语表的图文档行同口径。所以本 spec 不为图状态申请新的实例段，也不引入自己的数据分类。仍待定的是落盘粒度：状态文档与实例快照合成一份还是两份，建议合成一份、同一事务落盘，理由见第 11 节 Q5。

## 7 校验与失败

按 C17 的三级归类，本 spec 的具体落点如下。每个问题项都含种类、定位（图的 ContentId / nodeId / 边下标 / payload 字段名）与细节文本（C18）。

### 7.1 加载期 fail-fast

| 问题种类 | 定位信息 | 说明 |
| --- | --- | --- |
| DuplicateGraphId | graphId | 注册表内重复 |
| GraphKindUnregistered | graphKind + graphId | 内容声明了一种代码里没有的图业务 |
| UnknownSchemaVersion | schemaVersion | 只有版本 1 |
| IllegalNodeId | nodeId + 字符位置 | 字符集违规 |
| DuplicateNodeId | nodeId | 图内重复 |
| EdgeTargetMissing | 边下标 + 缺失的 nodeId | 悬空边（K31） |
| DuplicateEdge | 边下标 + (from, to) | 平行边 |
| SelfLoopForbidden | nodeId | 仅无环策略 |
| CycleDetected | 每个环成员的 nodeId | 无环策略下逐个报（K26） |
| PayloadMissing | nodeId | 节点没有 payload（K13） |
| NodeTypeNotWhitelisted | nodeId + nodeTypeId | 白名单外的类型（K9） |
| PayloadContentRefMissing | nodeId + payload 字段名 + 缺失的 ContentId | C13 第二段 |
| StateNodeUnknown | nodeId | 状态文档引用了已删除的节点（K40） |
| GraphDocMissing | graphId | 状态文档的图在注册表里不存在 |

### 7.2 允许的降级

| 问题种类 | 默认行为 | 开关关闭后 |
| --- | --- | --- |
| NodeUnreachable | 记录警告并继续装载（K30） | 升为 fail-fast |

本 spec 只有这一条降级。刻意不加更多：降级每多一条，就多一条「这份内容到底有没有被完整校验过」的疑问。

### 7.3 运行期断言

| 不变量 | 断言内容 | 发布期行为 |
| --- | --- | --- |
| IllegalTransition | 迁移 (源, 目标) 不在合法集合（K14） | 返回 `E_IllegalTransition`，状态不变 |
| TransitionRejectedBySemantics | 单调语义下收到回退请求（K16） | 返回 `E_TransitionRejectedBySemantics` |
| ActiveLimitExceeded | 超过语义声明的上限（K20） | 返回 `E_ActiveLimitExceeded` |
| StepCapReached | 一次 pass 超过步数上限（K27） | 结束 pass，返回 `StepCapReached` |
| TransitionOutsideVariablePhase | 在 Fixed 相位调用迁移入口（K41） | 返回失败，不执行迁移 |
| PayloadMutatedAtRuntime | payload 字段在装载后被改写（K12） | 返回失败并记问题项 |
| HookReentrancy | 钩子内再次触发同一实例的迁移 | 拒绝内层迁移，返回失败 |

### 7.4 钩子失败

| 情形 | 级别 | 处理 |
| --- | --- | --- |
| `OnCanActivate` 返回拒绝 | 正常业务结果 | 返回 Denied，无问题项 |
| `OnActivated` / `OnDeactivated` 返回问题项 | 运行期失败，可重试 | 记问题项 + 挂起事务 + 有界重试（K23） |
| 重试次数用尽 | 运行期失败，终态 | 返回失败码，保留挂起事务待下次装载 |
| 钩子抛出异常 | 同上 | 捕获后按上两行处理，异常不得穿出内核 |

## 8 可迁移性注记

去掉 C++ 与 minEngine 之后逐条判断。

| 条目 | 判断 | 说明与条件 |
| --- | --- | --- |
| 图文档 = 节点 + 边 + 图级元信息，只存结构（K1–K5） | 成立 | 图是数学对象；「布局与语义分离」与语言无关 |
| 一种图种类一个解析器注册项（K32–K34） | 成立 | 策略模式 + 注册表，任何语言都能表达 |
| 四状态迁移表 + 语义收窄（K14–K18） | 成立 | 状态机是纯数据与判定 |
| payload 多态承载与 nodeTypeId 白名单（K7–K9） | 条件成立 | 需要「可多态实例化的对象 + 按名字创建的白名单工厂」。目标引擎无反射时，退化为「判别串 + 扁平字段表」或「判别串 + 独立 JSON 文档」，白名单与「校验先于实例化」的顺序约束必须保留 |
| payload 内联于图文档（K7） | 条件成立 | 选内联是因为它没有独立身份，与反射能力无关；若目标平台的资产格式不支持嵌套对象，改成「图文档内的一张扁平 payload 表 + 节点持下标」，语义不变 |
| 无环策略 Kahn + 逐个报环成员（K26） | 成立 | 纯算法 |
| 允许环策略的运行期步数上限（K27） | 成立 | 纯计数器 |
| 「运行期绝不做环检测」（K28） | 成立 | 这是纪律，不是能力 |
| 派生状态不落盘 + 装载后重算（K37–K39） | 成立 | 值得无条件迁移的一条 |
| 挂起事务落盘与装载重试（K24） | 条件成立 | 需要「事务性副作用 + 可持久化的未完成记录」。目标平台若没有可靠落盘时机，至少要把挂起事务保留在内存并在卸载前强制结算，否则「状态已提交、副作用未做成」这条裂缝会变成数据丢失 |
| 状态写入单点 + 只读访问器（K33） | 成立 | 与引擎无关 |
| 图状态由持有者子系统管理（K42） | 条件成立 | 需要一个生命周期容器；缺则用显式生命周期的服务容器，所有权规则不变 |
| 图种类、nodeTypeId 是代码注册判别串（K2、K8） | 条件成立 | 依赖「语义必须有代码契约身份」这条原则（C8）；目标语言若允许运行期注册类型，仍建议保持「内容只能引用代码声明过的判别串」 |
| 作者坐标与标注不进规则（K5） | 成立 | 元信息与语义分离是通用做法 |

结论：没有一条依赖宿主反射实现的巧合。唯一真正受引擎约束的是 payload 的物理承载（内联还是扁平表），而它的契约（白名单、校验先于实例化、payload 是内容不是状态）在任一语言里都能原样成立。

## 9 与其它块的关系

### 9.1 依赖方向与需要对方提供的接口

依赖方向严格单向：F05 依赖 F02、F03、F06，且全部经核心层值类型与只读接口，不反向；CORE-F07 不在依赖清单里，它只是接线对象（见下表最后一行与下段）。

| 上游 | 本 spec 需要的接口 | 用途 | 边界 |
| --- | --- | --- | --- |
| CORE-F02 属性 | 只读查询：给定实例与 TagName，返回当前值 | 前置条件里的属性阈值（`CheckExtra` 与解析器的入边判定） | 只读；图内核不持有属性写入能力。派生状态依赖外部属性，这正是 Available 不能落盘的根本理由（4.6.3） |
| CORE-F03 内容定义与实例 | 按 ContentId 查定义；构造实例；两段式引用解析（C13） | graphId 解析、payload 内容引用解析、图实例的 tag 增量 | 图不自己实现注册表；图只向注册表查询 |
| CORE-F06 会话与子系统 | 子系统生命周期、实例句柄、加载闸口 | 图实例的持有者、装载时机 | 图不自带管理器单例（K42） |
| CORE-F07 能力运行期 | 授予与撤销接口 `GrantFromNode(owner, abilityId, nodeToken)` / `RevokeByToken(nodeToken)` | 节点激活、去激活、图卸载时把能力来源落到具体授予上 | **装配期经组合根接线，不是 include 依赖**：调用点在业务解析器实现里，图内核不 include 能力模块头、不持有其类型 |

关于 F07 的方向要说清：**图内核不依赖 CORE-F07。** 内核只提供钩子与值类型（9.4），「钩子实现里调用了能力运行期」是业务解析器加组合根的事，因此 `mgf-core` 的依赖清单里没有 F07；这与 CORE-F07 K13「能力运行期不 include 图模块头」互为反向确认 —— 两侧都不 include 对方，接线只发生在组合根与业务层。

被依赖方向：具体的技能树 / 科技树 / 任务 / 对话模块依赖本 spec 的解析器接口与迁移入口。它们不依赖内核实现，只依赖接口。

### 9.2 谁持有真值

与 CORE-F01 C31「图文档与派生状态 | 图实例（由持有者子系统管理）| 随实例」一致。补充三条：

1. 图文档（结构 + payload）的真值在**内容侧**：注册表返回 const 语义，运行期无人写它（C4）。
2. 图状态表（已决定状态 + 挂起事务）的真值在**图实例**：写入只有一个入口（K18 / K33）。
3. 派生状态（Locked / Available）没有真值，它是每次重算出来的投影。任何「缓存起来的 Available」都是本 spec 明确要消灭的形态。

### 9.3 与三类块的具体交叉点

- **与 F03 的 def / instance**：图文档是一种内容定义，携带 ContentId 与 schemaVersion；图实例是实例（三个增量段）+ 图状态段。按 CORE-F03 §4.3 的族表，图文档落在「图定义族（kind = graph）」一行，资产子类与准入表由该族提供。**payload 不是内容定义**（K7 不变）：它是节点内联对象，不进注册表、不占 ContentId、不作为独立资产装载，其类型表与白名单由本文自己管。payload 属于定义侧，运行期只读（K12）。
- **与 F02 的属性**：属性阈值是「结构之外的门槛」，只能进 `CheckExtra` 或解析器的入边判定，绝不进图文档的边字段（K4 的理由）。属性变更不自动触发图重算：业务在属性变更通知后显式请求一次重算，理由是 C24「事件只做冒泡」与「不依赖广播顺序」（C27）。
- **与 F06 的所有权**：技能 / 科技树的图实例按 C30 归 GameInstance 子系统（跨场景、随会话）；对话图随 Scene 走。这一选择由使用方做出，框架只要求「有且只有一个持有者」。

### 9.4 本 spec 向使用方提供的接口

这一节是 9.1「我需要什么」的对偶：谁要接图，就从这里取。CORE-F07（能力运行期）需要的东西全部落在这几条上。

```cpp
// Sketch only; not the final signature.

/// @brief Identity of one graph instance; the same handle the owning subsystem holds.
using MGFGraphInstanceId = MGFInstanceId;

/// @brief Identity of one node inside its graph instance. Value semantics; never compared across instances.
struct MGFNodeId
{
    std::string m_Value; ///< The authored node id string.
};

/// @brief Ability source token for a graph node: which graph instance, which node.
struct MGFGraphNodeToken
{
    MGFGraphInstanceId m_Instance; ///< Owning graph instance.
    MGFNodeId m_Node;              ///< Node inside that instance.
};
```

| 提供项 | 形态 | 用途 | 约束 |
| --- | --- | --- | --- |
| `MGFGraphInstanceId` | 复用 CORE-F06 的实例句柄（本文草图中的 `MGFInstanceId` 同义） | 来源令牌的「哪个图」 | 不另造图侧 id；图实例销毁后该值不再指向任何图 |
| `MGFNodeId` | 包住 nodeId 串的值类型，可拷贝、可哈希、可比较 | 来源令牌的「哪个节点」 | 只在所属图实例内有意义；跨图比较恒为假 |
| `MGFGraphNodeToken` | 上述两者的值组合 | 能力来源令牌里的「图节点来源」字段 | 纯值语义：不含指针、可存进业务容器、可直接比对 |
| `OnCanActivate` / `OnActivated` / `OnDeactivated` | 解析器钩子（4.3.5） | 激活前准入、激活后施加、离开 Active 后撤销 | 全部在 Variable 相位；提交前与提交后分明；回退与完成两条路径都会触发 `OnDeactivated` |
| `OnGraphUnloading` | 解析器钩子（本节新定） | 图实例卸载 / 销毁前的一次通知 | 见下 |
| `QueryNodeState` / `EnumerateNodesInState` | 图实例上的只读查询 | 卸载时枚举当前 Active 节点，或运行期做只读检查 | 只读、无副作用、不触发钩子、可在任意相位调用 |

卸载钩子的结论是**提供**，理由与边界写死在这里：

- 需求是真实的：能力的授予按来源令牌记在能力运行期，图卸载后这些来源已经不存在；不通知就会留下一批来源悬空的能力，而能力侧无法自己发现（CORE-F07 K13 明令它不读图结构）。
- 通知只做两件事：告诉业务「这个图实例要走了」，并让业务在钩子里用状态查询枚举 Active 节点、逐个按令牌撤销。内核不替业务撤销任何东西，也不因此改变任何节点状态（Done 不回退，4.3.4）。
- 形态：卸载前调用一次，同一实例不重复调用；钩子内不得提交图迁移、不得落盘。卸载发生在非 Variable 相位（会话销毁）时同样调用，但此时只允许做撤销这类不依赖状态机的清理。
- 不提供逐节点的卸载钩子：卸载不是状态迁移，把 N 个节点各调一次会变成 N 次通知、N 个失败点，而业务真正需要的只是那张 Active 名单。

## 10 实现波次与验收判据

本 spec 落在 CORE-F01 §9 的 **Wave3（Graph）**，只依赖 Wave1 已交付的内容注册表、实例形态、问题列表与加载闸口。前置条件：Wave1 的 C1 / C5 / C11 / C13 / C17 / C19 用例已绿。

### 10.1 交付物

- `mgf-core`：图文档结构值类型、状态机与迁移表、四状态与请求枚举、状态表与挂起事务、校验策略接口与两个实现（无环 / 允许环）、解析器接口、问题种类。
- `mgf-engine-adapter`：节点与 payload 的反射类型、payload 白名单工厂、图文档与状态文档的序列化与版本闸、两段式引用解析的图侧接线。
- 组合根：`MGFGraphKindRegistration` 的注册入口（至少注册一个示例图种类）。
- `mgf-tests`：下列判据对应的 doctest 用例 + ctest 注册。

### 10.2 验收判据（全部可执行）

1. **`ctest` 全绿**，至少覆盖 K1、K3、K4、K9、K13、K14、K16、K23、K26、K27、K37、K39 各一条用例。
2. **坏图全部装载期拒绝并给定位**（对应 CORE-F01 Wave3 判据 1）：一份同时含「重复 nodeId + 悬空边 + 白名单外 nodeTypeId + payload 缺失 + 无环图含环」的夹具，返回的问题项数量等于错误数（不少于五条），每条含 nodeId 或边下标。
3. **派生状态不写盘**（对应判据 2）：状态文档全文不含 Available 与 Locked；对账用例中「装载后重算」与「从空状态逐帧推进」逐节点相等。
4. **一种图对应一种解析业务**（对应判据 3）：新增第二个 graphKind（对话图）的变更集里不含 `mgf-core` 目录下的文件（用 `git diff --name-only` 检查）。
5. **payload 内容引用走 ContentId 并通过 C13 解析**（对应判据 4）：构造引用两个不存在 id 的 payload，返回两条问题项。
6. **单调与流两种语义共存**：同一份图文档分别以单调与流语义注册，回退迁移分别返回 `E_TransitionRejectedBySemantics` 与成功；单调实例的 Done 集合在全部用例中只增不减。
7. **运行期无环检测、无重活**：一帧内图推进期间，`Validate` 与反射实例化调用计数均为零（计数器用例）；Fixed 相位调用迁移入口返回失败并断言。
8. **序列化往返等价**：图文档与状态文档各自 save → load → save 字节序列相等；把 schemaVersion 改为 0 与 2 各被拒绝一次。
9. **核心层纯净**：`mgf-core` 图相关源码的引擎符号 grep 为空（CORE-F01 §4.2 检查项）；源码 ID 与章节号 grep 为空（C43）。

## 11 待决问题

**Q1 需不需要第五个状态或 Done 的结果维度。** 选项：(a) 维持四状态，失败的语义由业务写实例 tag 增量表达；(b) 增加 `Failed` 状态；(c) `Done` 携带一个 outcome 枚举。
建议 (a)。理由：状态是内核概念，「成功 / 失败」是业务判断；多了它，内核就要理解「什么算失败」，而这在不同业务里定义完全不同（对话图根本没有失败）。什么会推翻建议：若三种业务的完成语义都需要在存档里被外部工具（成就系统、进度统计）直接读取，把 outcome 放进状态文档更省事，此时选 (c)。

**Q2 边要不要带条件。** 选项：(a) 不带，条件进节点 payload 的 `CheckExtra`；(b) 边带一个条件 tag + 参数；(c) 边带一个条件表达式字符串。
建议 (a)。理由：条件是业务语义，且数量形态由业务决定；进结构层意味着内核要理解条件表达式，解耦立刻失效。(c) 还要额外的表达式求值引擎，与「不做对话运行时脚本」的非目标冲突。什么会推翻建议：若出现「同一对节点之间需要多条互斥边」的真实内容需求，把 (b) 作为正式契约补进来。

**Q3 反激活要不要级联到已完成后继。** 选项：(a) 内核不级联，Done 永不回退；(b) 反激活按依赖顺序级联把 Done 退回。
建议 (a)。理由：(b) 要求业务为每个副作用提供逆运算，而授予技能、发奖励都可能不可逆；一旦有一个逆运算写漏，存档与运行时就会长期矛盾。什么会推翻建议：若某类图的所有副作用都可逆且有统一的逆运算接口，级联可以作为一个显式的「批请求」由解析器实现，内核不必变。

**Q4 挂起事务的重试上限与到期处理。** 选项：(a) 上限 3 次，用尽保留挂起事务并返回失败码；(b) 上限 3 次，用尽后把状态回滚到迁移前；(c) 无上限，一直重试。
建议 (a)。理由：(b) 又回到逆运算问题；(c) 会让一个必然失败的事务在每帧重试，把一次性错误变成性能问题与日志洪水。什么会推翻建议：若副作用失败在实践中总是可恢复的（例如都是内存分配），(c) 配退避更简单。

**Q5 图状态段与实例快照是一份文档还是两份。** 选项：(a) 合成一份文档（同一实例一份 JSON，图状态是其中一段）；(b) 图状态单独一份文档，按实例 id 关联。
建议 (a)。理由：两份文档就需要跨文档事务，否则会出现「实例存了、图没存」的半状态；单一文档天然原子。什么会推翻建议：若一个实例上挂十几张图（大型科技树 + 任务 + 对话同时存在），单文档会变得很大且每次保存都全量重写，此时按图拆文档 + 显式事务。

**Q6（已关闭：总纲 R2 的 A3 解决）总纲 §3 对 GraphDoc 的措辞与「Active / Done 不是派生」冲突。** 观察（记录当时的措辞，现已不成立）：CORE-F01 §3 把 GraphDoc 描述为「节点 id 集合 + 边集合 + 节点 payload 引用 + 一张**可丢弃的**派生状态表」，而本 spec 的结论是 Active / Done 是决策、不可丢弃，只有 Locked / Available 可丢弃。与 C11「派生状态一律不写盘」没有冲突，与 C31 的真值表也没有冲突，冲突只在「可丢弃」这个词上。
建议：保留本 spec 的区分（决策状态 + 派生状态），由作者在下次修订总纲时把 §3 的措辞改为「一张可丢弃的派生状态表 + 一份必须持久化的决策状态表」。不建议本 spec 自行改总纲。什么会推翻建议：若作者认为「决策状态」属于 C5 实例三段之外的第四段、需要先进总纲取得身份，则本 spec 的 K37 需要重写为「决策状态作为实例的一个片段」—— 这是备选方案，代价是图状态要挤进 fragment 覆盖列表的表达能力里。 **处理结果：已关闭。** 总纲 R2 的 A3 已把 §3 术语表与 §6 表格的图文档行改写为「节点状态表（决策状态 Active/Done 必须持久化，派生状态 Locked/Available 加载后重算）」，措辞冲突消除，观察里的「可丢弃」已不再是总纲的表述；R3 的 B2 又把这句写进 C11 正文，本文 K37 / K38 与 6.5 据此对齐。备选方案（把决策状态当成 C5 之外的第四段）不再考虑。

**Q7 不可达节点的默认级别。** 选项：(a) 允许的降级（默认警告，开关可升为 fail-fast）；(b) 一律 fail-fast。
建议 (a)。理由：不可达节点是内容完整性问题而非一致性问题，且作者经常会先摆好后续节点再接边。什么会推翻建议：若实践中发生过「不可达节点导致线上行为与设计不符且无人发现」，升为 (b)。

**Q8 payload 是否允许被多张图共享。** 选项：(a) 不允许，payload 内联于唯一的图；(b) 允许，payload 变成独立资产或独立定义。
建议 (a)。理由：共享 payload 立刻引入引用者与被引用者的生命周期耦合、以及「改一处影响多图」的隐蔽风险，而当前没有任何真实需求。什么会推翻建议：若出现「同一段对话片段被多张对话图复用」的真实需求，按 (b) 设计，但共享单位应是**子图**而不是 payload —— 子图有结构，允许复用的价值才成立。

**Q9 对话图的重复访问次数怎么存。** 选项：(a) 实例 tag 键增量表（C5）；(b) 图状态段里给每个节点加一个计数。
建议 (a)。理由：(b) 会让状态文档从「决策集合」退化成「统计表」，而这些统计本来就是业务数据，与图的结构判断无关。什么会推翻建议：若内核自己需要计数来做判定（例如「最多重复三次」），那它就成了决策的一部分，应进状态文档并新增契约。

**Q10 图种类判别串与 ContentId 的 kind 段是否合并。** 选项：(a) 分开：ContentId 的 kind 固定 `graph`，graphKind 是另一个字段；(b) 合并：kind 段直接就是 graphKind。
建议 (a)。理由：ContentId 的 kind 决定注册表分片与资产类型（C1），图的分片就是 `graph` 一个；把 graphKind 塞进 kind 会让注册表按业务分片，新增一种图业务就要改注册表分片逻辑，正是「新增业务不改内核」要避免的。什么会推翻建议：若注册表本来就想按图业务分开索引（例如按业务分别装载），合并更省一层。

**Q11 内容 id 引用值类型的最终定名以 CORE-F03 为准。** 审查裁定 B15 要求本文不再自定内容 id 的编码、一律改用 CORE-F03 的 `MGFContentIdRef`，本文已照此执行（4.1 与 6.2 / 6.3 的草图按该类型的序列化形态理解）。本文不掌握该类型在 F03 里的最终拼写：若定名不同，改的是本文的引用名与草图标签，语义与契约条目不变。这一条不需要决定，只是把跨文档依赖记在明处。

## 修订记录 R3

本节记录本轮跨文档审查裁定的落地。只做定点修改：改句子、补括注、加一条契约、加一段说明；章节编号与顺序未变。

| 审查编号 | 改了什么 | 位置 |
| --- | --- | --- |
| B2 | 删去「第四块数据」这类自造分类，改为引用 C11 的归位（决策状态与挂起事务属允许落盘的决策数据，Locked / Available 不落盘）；K37 / K38 各补一句指向 C11 | 6.5、K37、K38 |
| B10 | 9.1 依赖表增 CORE-F07 一行（授予 / 撤销接口，装配期经组合根接线、不是 include 依赖），并补一段明说图内核不依赖 F07 | 9.1 |
| B14 | 6.4 增一条版本政策：内容资产版本只记录、持久化文档 schemaVersion 必须分支；图文档与图状态文档属后者 | 6.4 |
| B15 | 资产侧内容引用统一改用 CORE-F03 的 `MGFContentIdRef`，删去本文自定编码的表述；4.1 的 graphId 行同步加括注 | 4.1（表行 + 说明段） |
| B17 | 确认 payload 不是内容定义（K7 不变），并在 9.3 写明图文档落在 CORE-F03 的「图定义族（kind = graph）」行、payload 不进注册表 | 9.3 |
| D 表提供物 | 新增 9.4「本 spec 向使用方提供的接口」：图实例 id 与节点 id 值类型、三个生命周期钩子、卸载钩子 `OnGraphUnloading`（结论是提供）、只读状态查询 | 新增 9.4 |
| G2 | K31 改为引用 C17 的澄清句（已声明的图边缺失恒为 fail-fast，不属降级），结论不变 | K31 |
| B18 | 11 节 Q6 标记为已由总纲 R2 的 A3 解决并关闭 | 11 Q6 |
| 附加（B15 连带） | 11 节新增 Q11，记录 `MGFContentIdRef` 的最终定名以 CORE-F03 为准这一跨文档依赖 | 11 Q11 |
| 附加（本节） | 新增本节，便于回溯本轮改了哪几处、为什么只改这几处 | 本节 |

## 12 评审意见（作者填写）

使用说明：

- 作者只填三列：**位置**（章节号或行内引用，例如 `4.3.2`、`K16`、`第 11 节 Q3`）、**作者意见**（一句话说清问题或不同看法，不必给方案）。
- **判断依据**与**处理**两列由设计者回填：判断依据写「为什么当初这么定」（契约条文、引擎事实、外部经验、代价权衡）；处理结论三选一——**接受** / **反驳 + 理由** / **转待决问题 Qn**。
- 一行一条意见；不同意就直接写，不需要组织语言。
- 处理完成后，作者只需复核「反驳」与「转待决」两类行。

| # | 位置（章节号/行内引用） | 作者意见 | 判断依据（由设计者回填） | 处理（接受 / 反驳+理由 / 转待决问题 Qn） |
|---|------------------------|---------|------------------------|------------------------------------------|
|   |                        |         |                        |                                          |
|   |                        |         |                        |                                          |
|   |                        |         |                        |                                          |
|   |                        |         |                        |                                          |
|   |                        |         |                        |                                          |
|   |                        |         |                        |                                          |
|   |                        |         |                        |                                          |
|   |                        |         |                        |                                          |
|   |                        |         |                        |                                          |
|   |                        |         |                        |                                          |
