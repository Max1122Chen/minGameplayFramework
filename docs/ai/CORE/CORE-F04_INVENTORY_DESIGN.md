# CORE-F04 背包（Inventory）

> 状态：Draft
> 作者：项目作者
> 最后更新：2026-10-26
> 关联契约：C5、C6、C7、C8、C9、C11、C12、C13、C14、C17、C18、C19、C20、C21、C22、C24、C25、C26、C28、C29、C31、C36、C38、C40、C43、C44、C45、C46（总纲第 5 节；与之冲突的设想一律进第 11 节，不自行改契约）
> 上游特性：CORE-F01（框架分层与跨块契约，本 spec 的唯一契约源）
> 同级依赖：CORE-F03（内容定义：ContentId、实例类与有效片段视图、定义侧片段、实例构造）、CORE-F02（属性与效果：物品实例上的 tag 增量与效果影响；容量不经属性，见 C48）、CORE-F06（会话与子系统：持有者与生命周期）
> 下游消费方：装备、交易、合成、只读 UI、世界掉落执行器（只接收溢出请求）

## 1 问题与动机

背包是整套框架里最容易被实现细节污染的一块，也是外部队列里唯一有「已落地后又回头重写」证据的一块。参考实现宿主上的物品栏曾经长成「槽的数组 + 只进不出的辅助函数」：只有 Insert 没有 Remove、槽位没有语义身份、每个实例都把耐久硬绑在通用实例上、槽位常量一半用枚举一半用裸下标、掉落生成逻辑被写进容器内部。这些形态单看都能跑，合起来就是三件事同时失控：**接口语义不清**（调用方不知道 TryAdd 到底收下了多少）、**策略不可派生**（上限与扩容写死在基类）、**几何被塞进槽位**（多格物品无处安放，最后把网格坐标混进槽位索引）。

本 spec 要在总纲 C5（实例三段形态）、C21（结构化结果）、C29（单一写入点）之上把背包这一块钉死，回答五个必须回答的问题：

1. 基类到底给什么、不给什么。只给**接口语义**：入口、结果形态、失败原因、原子性承诺、不变量；不给上限、不给扩容、不给几何。
2. 容量与准入为什么必须是派生 + 规则、而不是几个数据字段。无上限 / 有上限 / 可扩容 / 只收特定种类 / 只收特定区域这五种需求，只有前者能用同一套契约测试覆盖。（容量的**数值**仍是容器定义里的整数字段，见第 4.6 节；这里说的是**判定规则**不能是一堆数据字段，也不能是属性。）
3. 槽位的语义身份从哪来。身份就是 index；具名区域（region）表达 hotbar / storage / 装备位 / 副手 / 工作台输入输出，且区域必须是代码而不是数据。
4. 几何与槽位如何彻底解耦。占位形状来自物品实例的片段，二维网格是派生类成员，两者是两套坐标。
5. 状态怎么存、一次移动怎么保证不复制物品。这是本 spec 的验收核心：**不存在需要回滚的事务**，是设计目标而不是巧合。

### 1.1 事实与既往经验基线

| 事实或经验 | 对本 spec 的含义 |
| --- | --- |
| 参考实现宿主（minEngine）没有背包、没有 GAS，反射属性无 inline struct | 背包全自建；槽位记录与物品实例必须是反射派生对象（Instanced），不能是内联结构体（细化 C9） |
| 引擎执行模型单线程同步 | 事务不需要锁；「先算后提交」只在单次调用内成立（C22 保证变更只在 Variable 相位） |
| 既往实现的 TryAdd 返回语义含糊，靠调用方猜剩余量 | 结果必须是结构化的：收下的量、拒收的量、拒收原因、相关槽位（C21） |
| 既往实现把盔甲位 / 副手位纳入 TryAdd 的自动填充 | 「自动填充哪些区域」必须由容器显式声明，否则一次拾取会把装备顶掉 |
| 既往实现出现「先复制再删除」的两步搬运路径 | 所有搬运写成同一计划内的源写 + 目标写，禁止两段式补偿（第 4.4 节） |
| 既往实现的上下文都独立得出同一个结论：容器只发溢出请求、由场景侧执行掉落 | 溢出分工进契约（K22），容器不 include 世界层 |

## 2 目标与非目标

### 2.1 目标

- **G1 一套接口语义，三种容量策略共用。** 无上限 / 有上限 / 可扩容三个派生必须通过同一套契约用例（总纲失败模式 9 的验收面）。
- **G2 结果结构化、失败可归因。** 每个变更入口返回结构化结果；拒收必带原因；不存在语义为「是否成功」的布尔公开函数（C21）。
- **G3 事务无回滚面。** 每个变更入口内部是「构建计划 → 全量校验 → 提交」，提交阶段不失败、不分配、不回调；代码里没有补偿路径。
- **G4 槽位身份与区域语义。** 槽位身份是 index；区域以单一声明表表达，边界由累加推导，越界与重叠在装载期 fail-fast。
- **G5 几何与槽位解耦。** 基类不知道网格存在；占位形状来自实例片段；二维网格是派生类成员；「一槽一格」使放置退化为槽位写入。
- **G6 可序列化的最小集。** 持久化只含「槽位索引 + 持有物 ContentId + 数量 + 实例增量 + 覆盖列表 + 放置朝向」，派生量一律不写盘（C11）。
- **G7 溢出只发请求。** 容器不认识世界；掉落、转入父容器由持有方策略决定。

### 2.2 非目标

- **不做 UI 与拖拽交互。** 不定义光标携带态、拖拽中途态、网格高亮、拖拽取消。这些是交互层对 `TryQuery` 与 `TryMove` 的组合调用；本 spec 只保证被调用的一侧语义正确。
- **不做物品耐久消耗逻辑。** 耐久是物品实例上的一个增量键 + 消耗效果，属 CORE-F02 与内容侧；背包只负责搬运，不读也不写耐久语义。
- **不做容器整理 / 排序 / 自动堆叠压缩。** 槽位索引是持久化身份，压缩会重映射所有索引，是丢物品的高发路径；需要时另开显式操作。
- **不做掉落物的世界表现。** 不生成实体、不给掉落速率与抛散算法；容器只交出溢出请求。
- **不做存档槽、自动存档与跨版本迁移。** 与 C15 一致，只保证实例状态快照自洽往返。
- **不做网络同步与并发安全。** 与总纲一致，第一版按单线程同步假设。
- **不定义物品内容。** 物品种类、堆叠上限的具体数值、装备槽位的 tag 词汇表属 CORE-F03 与内容包；本 spec 只定「从哪读、怎么校验」。
- **不定义配方匹配与产物预览规则。** 工作台输出槽在本 spec 里只是「一个只读区域」，匹配算法属合成块。
- **不做多格物品跨槽。** 一件物品的占位由网格覆盖表达，不允许「一件物品横跨多个槽位索引」。

## 3 术语

只列本 spec 新增概念；ContentId、TagName、Definition、Instance、Fragment、问题项 / 问题列表、失败三级分类、schemaVersion、RAII 订阅句柄、tick 相位规则一律沿用总纲 §3 与 §5，本文不另起定义。

| 术语 | 定义 |
| --- | --- |
| 槽位索引 SlotIndex | 无符号整数，槽位在容器内的身份与寻址单位；容器生命周期内稳定，不因内容增删而重排 |
| 槽位记录 SlotRecord | 一个槽位的全部状态：`空` 或 `{ 物品实例引用, 数量 }`；几何字段不在其中（第 4.9 节） |
| 容器定义 ContainerDefinition | 描述容器数值与装配关系的只读内容定义（整数字段：槽数参数即容量、准入 tag 集合、网格尺寸、溢出表现参数），属 CORE-F03 的定义族 |
| 容量策略 CapacityPolicy | 回答「还能放几个 / 这个槽位能不能放 / 能否扩容」的规则对象；是代码，不是数据字段（第 4.6 节） |
| 准入过滤器 AdmissionFilter | 回答「这种东西允许进这个容器 / 这个槽位吗」的规则对象；与容量策略正交（第 4.7 节） |
| 区域 Region | 容器内一段连续槽位的具名语义段（hotbar / storage / 装备位 / 副手 / 工作台输入 / 工作台输出） |
| 区域声明表 RegionSpec | 派生类里唯一一张 `{ 区域 id, 槽位数 }` 有序表，区域边界由累加推导 |
| 自动填充区 AutoFillRegions | 容器声明的、`TryAdd` 可以自动占用的一组区域；未声明的区域只能显式 `TryAddTo` |
| 占位形状 Footprint | `宽 × 高` 与「允许的朝向集合」；由物品实例的片段给出，属物品，不属容器 |
| 放置记录 Placement | 网格容器的附加槽位状态：该槽物品当前的朝向；派生量（占用格集合）可由它重算 |
| 网格容器 GridInventory | 持有二维网格的容器派生类；网格与槽位索引之间是它自己定义的双射 |
| 操作计划 OpPlan | 一次变更入口内部构建的、待提交的槽位写入集合；构建与校验均可失败，提交不可失败 |
| 拒收原因 RejectReason | 结构化结果里说明为何拒收的枚举值，清单见第 7.3 节 |
| 条目结果 EntryResult | 结构化结果里的一项：被作用的条目（槽位或物品条目）、该条目自己的结果码、拒收原因与接受/拒收量；单条操作的结果里恰好一项（第 4.3 节） |
| 溢出请求 OverflowRequest | 容器装不下的那部分物品 + 原因 + 表现参数；容器只产出请求，不执行掉落 |
| 变更代数 SlotGeneration | 每个槽位一个单调计数，仅运行期存在，用于接收方判断「哪些槽位变了」而不需要载荷（第 4.11 节） |

## 4 设计

### 4.1 分层与文件落点

| 内容 | 落点 | 理由 |
| --- | --- | --- |
| SlotIndex、RejectReason、InventoryOpResult、Footprint、朝向枚举、RegionSpec 与区域推导、CapacityPolicy / AdmissionFilter 抽象、SlotRecord 值形态、变更代数 | 核心可迁移层（一个概念一个头文件） | 全是规则与值，剥离引擎后仍然成立；满足 C45 与总纲 §4.3 判据 |
| 容器派生类（无上限 / 有上限 / 可扩容 / 网格）、容器定义资产、实例侧片段读取、事件通道适配、序列化读写 | 引擎适配层 | 需要反射派生、DataAsset、Serializer、事件总线 |
| 容器持有者、装载闸口上的容器快照与重建、掉落执行器绑定 | 组合根与会话层（CORE-F06） | 谁持有真值属 C31 的表，不属本 spec |

判据沿用总纲：核心层头文件里每个公开类型的字段类型都能用「另一种语言重写为等价 struct」描述；出现反射宏、资产句柄、引擎容器别名即越界（C46）。

### 4.2 槽位、槽位记录与不变量

槽位身份就是 index。容器内部是**一段连续的槽位记录**，不是「槽位对象数组」——不额外包一层 Slot 类，也不需要槽位对象持有引用。理由：槽位对象会诱使实现把「槽位自己的状态」（锁定、冷却、UI 高亮）写进容器，从而出现第二处真值；这些状态应挂在派生的容器侧或 UI 侧。

#### 4.2.1 数据形态

（示意，不是最终签名）

```cpp
/// @brief One slot's content. Empty means the slot holds nothing.
struct SlotRecord
{
    /// @brief Content identity of the held item; meaningless while the slot is empty.
    ContentId Definition;
    /// @brief Units held in this slot; zero while the slot is empty.
    std::uint32_t Count = 0;
    /// @brief Definition-provided instance class; the slot never restates instance semantics,
    ///        and its effective fragment view is read on demand instead of cached here.
    InstanceContent Content;
};

/// @brief Read-only view a caller receives; it never exposes a writable slot.
struct SlotView
{
    bool        bOccupied = false;
    ContentId   Definition;
    std::uint32_t Count = 0;
};
```

槽位里放的实例就是 C5 的三段形态（定义引用 + tag 键增量 + 覆盖列表），其类型是 CORE-F03 提供的实例类 `InstanceContent`：背包不自造字段，也不自持实例状态定义。**背包从不直接读 DataAsset 字段**，而是读 F03 提供的有效片段视图（`InstanceContent::GetFragmentView()` 返回的 `FragmentView`：定义装配 + 实例覆盖列表合并后的结果，按片段种类 `Find` / `TryGet` / `All` 读取），因此覆盖列表能改写堆叠上限与占位形状，而背包不需要知道覆盖机制（D1 裁定）。

#### 4.2.2 不变量

以下不变量由容器自己维护，违反即运行期断言（C17 第二级）：

1. **单持有**：一个物品实例在同一时刻只被一个槽位持有。容器内不保存共享实例引用，跨容器搬运一律值拷贝（C36 的「不传裸指针作长期引用」）。
2. **数量正值**：非空槽位的数量恒大于零；数量降到零即视为空槽，不存在「数量为 0 但占了位置」的中间态。
3. **数量不超过上限**：单槽数量不超过其堆叠上限（上限来源见第 4.10 节）。
4. **区域覆盖完整**：`0 .. SlotCount-1` 每一段都恰好归属一个区域，既不重叠也不留空洞。
5. **槽数单调**：容器存活期间 `SlotCount` 只增不减；扩容即追加尾部槽位。
6. **总量守恒**（调试期）：一次操作前后容器内各定义的总数量之差，等于该次操作显式声明的输入或输出量（第 4.4 节，dupe 探测）。

### 4.3 接口语义层：入口、结果与失败原因

基类只给契约，不给实现。下面是接口语义的示意（不是最终签名）：

```cpp
/// @brief Outcome of one entry. An entry is the smallest unit the operation acted on:
///        a slot, or one item entry inside a container.
struct EntryResult
{
    /// @brief Slot this entry touched; kInvalidSlotIndex when the entry is not slot bound.
    SlotIndex     Slot = kInvalidSlotIndex;
    /// @brief Ok or Rejected for this entry; an entry is never half accepted.
    InventoryCode Code = InventoryCode::Ok;
    /// @brief Why this entry was refused; None only pairs with Ok.
    RejectReason  Reason = RejectReason::None;
    /// @brief Units taken in or given out for this entry.
    std::uint32_t Accepted = 0;
    /// @brief Units refused for this entry; the caller owns them afterwards.
    std::uint32_t Rejected = 0;
};

/// @brief Outcome of one inventory mutation. Callers must inspect it; ignoring it asserts in debug builds.
struct InventoryOpResult
{
    /// @brief Aggregate outcome: Ok, PartiallyAccepted or Rejected.
    InventoryCode       Code = InventoryCode::Rejected;
    /// @brief Blocking reason of the whole operation; a single-entry operation mirrors its only entry here.
    RejectReason        Reason = RejectReason::None;
    /// @brief Units the container actually took in or gave out, summed over entries.
    std::uint32_t       Accepted = 0;
    /// @brief Units the container refused, summed over entries; the caller owns them afterwards.
    std::uint32_t       Rejected = 0;
    /// @brief Slot the operation targeted; kInvalidSlotIndex when it addresses several slots.
    SlotIndex           Slot = kInvalidSlotIndex;
    /// @brief One entry per slot or item entry touched; a single-entry operation has exactly one,
    ///        and a bulk operation lists every refused entry here instead of only an aggregate.
    std::vector<EntryResult> Entries;
};

/// @brief Container contract. Owns no capacity rule, no admission rule and no grid.
/// @warning All mutating entry points must be reached from the variable tick phase only.
class InventoryBase
{
public:
    virtual ~InventoryBase() = default;

    /// @brief Slot count; monotonic for the container's whole lifetime.
    virtual std::uint32_t SlotCount() const = 0;

    /// @brief Read-only slot view; an out-of-range index yields an empty view.
    virtual SlotView Query(SlotIndex slot) const = 0;

    /// @brief Reports where the units would land without changing anything.
    /// @note This is the planning entry point: UI and scripts must ask before they act.
    virtual InventoryOpResult TryQuery(const InstanceRef& item, std::uint32_t count) const = 0;

    /// @brief Folds into stackable slots first, then fills declared auto-fill regions.
    virtual InventoryOpResult TryAdd(const InstanceRef& item, std::uint32_t count) = 0;

    /// @brief Places units into one explicit slot; ignores auto-fill regions entirely.
    virtual InventoryOpResult TryAddTo(SlotIndex slot, const InstanceRef& item, std::uint32_t count) = 0;

    /// @brief Removes units; the removed items are handed back through the result.
    virtual InventoryOpResult TryRemove(SlotIndex slot, std::uint32_t count) = 0;

    /// @brief Moves a whole slot into an empty or stackable slot of any container.
    virtual InventoryOpResult TryMove(SlotIndex from, InventoryBase& target, SlotIndex to) = 0;

    /// @brief Exchanges two slots, possibly across containers; all or nothing.
    virtual InventoryOpResult TrySwap(InventoryBase& other, SlotIndex a, SlotIndex b) = 0;

    /// @brief Splits part of one slot into a target slot; all or nothing.
    virtual InventoryOpResult TrySplit(SlotIndex from, std::uint32_t count,
                                       InventoryBase& target, SlotIndex to) = 0;

    /// @brief Subscribes to slot changes; the returned handle unsubscribes on destruction.
    virtual SubscriptionHandle ObserveChanges(ChangeObserver observer) = 0;
};
```

结果有两个粒度，必须分清：**整体**用 `Code` 表达 `Ok` / `PartiallyAccepted` / `Rejected` 三档，**逐条目**用 `Entries` 表达，每个条目只有 `Ok` / `Rejected` 两档并各自携带 `RejectReason`。批量入口（`TryAdd`、跨容器 `TryMove` / `TrySwap` / `TrySplit`）返回部分接受时必须逐条目列出被拒项及原因，聚合字段只作摘要、不得掩盖逐条目信息；单条操作的结果里恰好一项，其 `Reason` 与整体的 `Reason` 相同（C21 已按此粒度裁定）。

三个语义决定，必须写死：

1. **`TryQuery` 是唯一的试算入口。** 它不修改任何状态，是 UI 高亮、脚本预判、拖拽合法性提示的唯一依据。禁止派生类另开「CanAdd」之类的布尔入口（C21 的可检查项要求「语义为是否成功的公开布尔函数为零」）。
2. **`TryAdd` 的自动填充区由容器声明**，默认只含存储类区域；装备位、副手位、只读输出位、合成输入位都不在自动填充区。这条直接对应既往实现的缺陷（拾取顶掉装备）。
3. **`TryMove` 不内建交换。** 目标槽被占用且不可合并时返回 `Occupied`，调用方改用 `TrySwap`。理由：把「拖到占用槽即交换」写进 TryMove 会让 TryMove 的结果同时是两种操作，回滚面与通知面都翻倍；而交换本来就是一个独立入口。

失败原因清单与「为什么不用布尔」：见第 7.3 节。

### 4.4 原子性：两段式计划-提交

#### 4.4.1 规则

每个变更入口的内部结构固定三步，不可省略、不可调换：

1. **构建计划**：只读扫描，算出一组「槽位索引 → 新槽位记录」的写入集合，以及需要交出的物品（移除量、溢出量）。此阶段允许失败、允许提前返回。
2. **全量校验**：对计划整体做不变量与容量校验（含目标容器的准入与容量）。任何一条不通过即整体拒绝，**不应用任何一项**。
3. **提交**：把写入集合逐项赋给槽位。提交阶段**不失败、不分配、不回调、不查询注册表、不发送事件**。

由此得到本文档对「事务失败如何回滚」的答案：**没有需要回滚的事务**。回滚本身是 dupe 的来源——补偿写一旦漏掉一项或写错顺序，就是同一批物品存在两处；而单线程 + 无失败提交让补偿成为不必要的复杂度（YAGNI）。

#### 4.4.2 为什么禁止边遍历边改

在计划阶段就地修改槽位（例如「先删源槽，再尝试放到目标，放不下再放回去」）在本宿主上会产生实际后果：

- 目标容器可能与源容器是同一个，遍历时的下标会随着就地修改而漂移；
- 就地修改期间若触发通知（哪怕只是脏位置位），接收方可能读到「源已空、目标未满」的中间态，进而做出错误决策；
- 「放回源槽」这一步本身可能失败（源槽在此期间被同一次操作的其它分支占用），此时物品同时存在于「已从源移除」和「未进入目标」的空洞中，即丢失；反向实现就是 dupe。

#### 4.4.3 dupe 防护清单

| 编号 | 规则 | 为什么 |
| --- | --- | --- |
| D1 | 搬运一律在同一计划的写入集合里同时包含源写与目标写 | 不存在「已复制未删除」的可观察窗口 |
| D2 | 提交阶段不得失败 | 没有半应用状态，也就不需要补偿写 |
| D3 | 提交阶段不得回调、不得发通知、不得查注册表 | 回调里再改容器会破坏计划的前提（C29 单一写入点） |
| D4 | 通知在提交完成后统一发出，且按 (容器, 通道) 去抖（C28）；区域或槽位级差异由变更代数回查表达 | 接收方永远只看到一致的终态，且不依赖通知条数推断差异 |
| D5 | 禁止把实例的内部字段取地址后长期持有 | 槽位记录重分配会让指针失效，表现为「同一件物品凭空多出一份」或写坏内存 |
| D6 | 调试期守恒断言：操作前后各定义总量之差 = 本操作声明的进出量 | 任何一处漏写或双写都会被下一步操作捕获 |
| D7 | 溢出请求在提交之后发出；请求的投递失败不回改容器 | 容器已经一致，回收责任在调用方 |

守恒断言（D6）是本文档对 dupe 类 bug 的主要探测器：它不依赖对实现的假设，只依赖「数量会计」。运维层面也保留一条更便宜的检查——调试期统计本次会话的 `Σ(接受量) - Σ(交出的量)` 与所有容器实际总量的差，非零即断言。

### 4.5 数量与边界规则

| 输入情形 | 规则 | 结果 |
| --- | --- | --- |
| `count == 0` | 拒绝，不用「成功的空操作」表达 | `Rejected` + `CountInvalid`（调试期附断言，因为这是调用方 bug） |
| `count < 0` | 无符号入参不可表达；若上层用有符号数量，必须在入口处先拒绝 | `Rejected` + `CountInvalid` |
| `count > MaxStack`，走 `TryAdd` | 允许部分接受：拆成多堆依次落入自动填充区，直到装不下 | `Accepted = 实际接收`，`Rejected = 余量`，`Code = PartiallyAccepted` |
| `count > MaxStack`，走 `TryAddTo` | 单槽只接受至多 `MaxStack`，其余如实拒收 | `PartiallyAccepted` + `Rejected = 余量` |
| `slot` 越界 | 拒绝 | `Rejected` + `OutOfRange` |
| 源槽为空 | 拒绝，不返回「成功但取出零」 | `Rejected` + `EmptySource` |
| `count >` 源槽数量 | 拒绝整次操作，不做「有多少取多少」 | `Rejected` + `NotEnough`（`TryAdd` 的部分接受语义不迁移到移除侧） |
| 源 == 目标（Move / Swap / Split） | 幂等成功，不产生任何写入，不发通知 | `Ok`，`Accepted = 0` |
| 目标槽已占用且不可合并 | `TryMove` 拒绝 | `Rejected` + `Occupied`；调用方改用 `TrySwap` |
| 目标槽已占用且可合并 | `TryMove` 按堆叠规则合并，超出上限的留在源槽 | `PartiallyAccepted` + `Rejected = 留在源槽的量` |
| 数量已是上限时再堆叠 | 视为装不下 | `Rejected` + `CapacityExceeded` |

「移除不做部分接受」是有意的非对称：加入的语义是「尽量收下，剩下还给你」，移除的语义是「我要拿走 N 个」，拿不到就是失败。若允许部分移除，调用方必须自己比较结果与请求，反而更容易写错。

### 4.6 派生与容量策略

#### 4.6.1 为什么必须有派生

五种真实需求无法用同一组数据字段表达：

| 需求 | 例子 | 若用数据字段表达会怎样 |
| --- | --- | --- |
| 无上限 | 调试容器、脚本给的无限口袋 | 需要一个「无限」哨兵值，于是所有容量计算都要特判哨兵 |
| 有上限且固定 | 箱子 27 格、熔炉 3 格 | 可表达 |
| 可扩容 | 背包升级加行，行数是容器定义里的整数字段，扩容只经显式入口 | 槽数变成可变，而槽数又是索引上界，任何缓存上界的地方都要失效 |
| 只收特定种类 | 箭袋只收箭、药水袋只收药水 | 需要一张「种类白名单」，而白名单的判定逻辑仍是代码分支 |
| 只收特定区域 | 装备位只收对应槽位 tag、输出位只读 | 与上一条同构，只是判定维度换成区域 |

把这些塞进基类的成员（上限、扩容标记、白名单数组），就是总纲失败模式 9 的形态：派生类只能复制基类再改，行为分叉且无法用同一套契约用例覆盖。因此：**容量与准入是抽象接口（规则），派生类只提供槽位数量参数（数据）**。这正对 C7（数值与装配关系可数据化）与 C8（需要新代码分支的语义必须在代码里取得身份）。

容量的数值只有两个来源：容器定义里的整数字段（槽数；若将来出现按件数而非槽位计量的容量，同样是整数字段），和**显式的容量变更入口**（扩容、一次性变更）。**不存在「读属性当前值算容量」的路径**——属性值只有 `float`（C48），把槽数或容量建模成属性会让上限随修饰器生灭静默漂移，且没有一次「容量变了」的操作可供对账、可供通知（第 4.15 节）。

#### 4.6.2 策略接口

（示意，不是最终签名）

```cpp
/// @brief Decides how much a container can still take and whether it may grow.
class CapacityPolicy
{
public:
    virtual ~CapacityPolicy() = default;

    /// @brief Units of this item the container could still accept, ignoring admission filters.
    /// @return The remaining capacity in units; unbounded policies return a saturated maximum.
    virtual std::uint32_t RemainingFor(const InstanceRef& item) const = 0;

    /// @brief Whether one explicit slot may receive more units of this item.
    virtual bool AcceptsInto(SlotIndex slot, const InstanceRef& item, std::uint32_t count) const = 0;

    /// @brief Whether the container is designed to grow beyond its current slot count.
    virtual bool CanExpand() const = 0;
};

/// @brief Decides which items may enter a container or a single slot, independent of capacity.
class AdmissionFilter
{
public:
    virtual ~AdmissionFilter() = default;

    /// @brief Reports whether the item may enter the slot, with a reason when it may not.
    virtual AdmissionVerdict Admit(const InstanceRef& item, SlotIndex slot) const = 0;
};
```

扩容语义（关键决定）：

- 扩容**只追加尾部槽位**，`SlotCount` 单调增加，已有槽位索引永不改变。
- 允许扩容的容器在扩容时把新槽位加到它所属区域的尾部；若该区域不是当前最后一个区域，则拒绝扩容（`Rejected` + `RegionNotExpandable`）。理由：区域必须是连续段（不变量 4），把新槽位插到中间区域会让所有后续区域的索引整体位移，等于重映射持久化身份。
- 第一版不提供「缩容」与「压缩整理」（第 2.2 节）。若将来要提供，必须是显式操作 + 索引重映射 + 一次完整校验，不允许自动发生。
- **一次性容量变更也是显式入口**（`TryApplyCapacityChange`）：调用方一次调用、容器一次校验、一次提交、一条通知。它只改**可用容量上限**（准入判定读的那个整数），不增删槽位、不改任何槽位索引——槽位的增减只有扩容一条路径（不变量 5 与 K9 不因此松动）。上限可升可降；降上限使已持有量超过上限时容器进入 `OverCap`（第 4.15 节），绝不静默裁剪。
- **容量不从属性派生。** 若将来确实要「容量随效果变化」，唯一允许的路径是：按 C48 由内容声明取整方向（默认向零）把属性派生结果算成**整数字段**，再经同一条容量变更入口落地——即变成一次可对账、可通知的显式操作；容量判定处永远只读整数字段，不现读属性。理由见第 11 节 Q7。

#### 4.6.3 三种继承示例

（示意，不是最终签名）

```cpp
/// @brief Container without a capacity ceiling; useful for script-granted storage and tests.
class UnboundedInventory final : public InventoryBase
{
public:
    /// @brief Grows one slot at a time so the slot count is always sufficient for what it holds.
    InventoryOpResult TryAdd(const InstanceRef& item, std::uint32_t count) override;

private:
    // No capacity member exists here on purpose: an absent rule needs no sentinel value.
    std::vector<SlotRecord> m_Slots;
};

/// @brief Container with a fixed slot count taken from its container definition.
class FixedSlotInventory : public InventoryBase
{
public:
    /// @brief Slot count equals the sum of the region declaration table.
    std::uint32_t SlotCount() const override { return m_SlotCount; }

protected:
    std::uint32_t m_SlotCount = 0;
    std::vector<SlotRecord> m_Slots;
};

/// @brief Container whose slot count can grow; the target slot count comes from an integer field
///        in its container definition, never from an attribute value.
class ExpandableInventory : public FixedSlotInventory
{
public:
    /// @brief Appends slots to the last region; refuses when the target region is not last.
    InventoryOpResult TryExpand(std::uint32_t slotCount);

    /// @brief Applies one explicit, caller-driven change of the usable capacity limit; slots are
    ///        never added or removed here, and a lower limit only blocks further growth.
    InventoryOpResult TryApplyCapacityChange(std::int32_t capacityDelta);
};
```

三种派生共用同一套契约用例（第 10 节）：有上限与可扩容的容量数值来自容器定义里的整数字段，无上限派生没有容量数值、也不需要哨兵；差别只在策略对象与那个字段的值，因此同一套用例能原样跑三遍。可扩容容器不是「另一种基类」，是「有上限容器 + 一个可被调用的容量变更入口」。

### 4.7 准入过滤器

准入与容量正交：容量问「还有多少空间」，准入问「允不允许」。必须分开，否则「箭袋满了」与「箭袋不收剑」会返回同一个失败原因，UI 无法给出正确提示，内容作者也无法定位问题。

判定维度只有三个，且都由容器声明：

1. **种类维度**：容器的准入 tag 集合（tag 名字数组，C39/C40）。物品实例的片段视图给出它携带的准入 tag，交集非空即允许。
2. **区域维度**：某些区域只接受显式放置、不接受自动填充（不是「不允许进」，而是「不进自动填充」——两者必须分开，否则 `TryAddTo` 也会被一起封死）。
3. **槽位维度**：单槽专属约束（装备位只收对应部位的物品）。实现上仍是准入 tag 集合，只是集合来自该区域。

只读区域（工作台输出位、配方预览位）用同一条路径表达：它的准入过滤器一律返回拒绝，且该区域不在自动填充区。这样「只读」不需要基类提供任何标记，也不需要调用方特判。

### 4.8 槽位语义区域

#### 4.8.1 区域是代码还是数据

**判断：区域是派生类里的代码常量，不是内容数据。** 理由三条：

1. **可编译期保证的东西不该变成装载期问题。** 区域一旦数据化，就必须在装载期校验「不重叠 / 不越界 / 覆盖完整」，等于凭空增加一整类 fail-fast 问题项（C17），而这些错误本来不可能发生。
2. **区域的语义与代码分支耦合。** 工作台输出位只读、装备位有准入、合成输入位不进自动填充、hotbar 有选中态——这些都是行为分支。让数据决定行为违反 C8：需要新代码分支的语义必须先取得代码身份。
3. **区域的消费方是代码。** 取用写的是 `Equipment(Head)` 或 `CraftResult()`，不是查表；数据化唯一的收益是「编辑器里可改布局」，而编辑器面板正是非目标（总纲失败模式 8）。

**折中（数据仍有用）：** 容器定义只提供**数值参数**（如主袋行数、网格行数列数、装备位个数），派生类用这些数值算区域边界；算出来的边界在装载期校验一次（越界或与别处冲突即 fail-fast）。因此「hotbar 是 0..8」是代码，「主袋有 3 行」是数据。

#### 4.8.2 单一声明表（对既有 enum 偏移做法的改进）

既往实现的做法是「`enum class Region` + 一组 `kXxxBegin` 常量 + 每组一个 `Xxx(i)` 偏移函数」。它比裸下标好，但有四个具体缺点：

| 缺点 | 后果 |
| --- | --- |
| 区域的 begin 与上一段的 end 分两处写（`kMainBegin = 9` 与 `kHotbarBegin + 9`） | 两处真值，改一处就分叉，且没有任何机制能发现 |
| 不连续的小区域靠人手算（材料区 4 格后紧跟一个产物位） | 常量相加写错时只能靠在运行期越界才发现 |
| 每种容器一套风格（一组 `enum class Slot`，另一组 `enum class Region`） | 取用方式不同，调用方要记两套写法 |
| 子类枚举名隐藏基类的槽位访问器（同名 `Slot`） | 已被迫给子类改访问器名，接口开始发散 |

本 spec 改成**一张有序声明表 + 边界推导**：区域边界由「前面所有区域槽数之和」算出，不手写任何 begin/end。

（示意，不是最终签名）

```cpp
/// @brief Stable name of one semantic slot region inside a container.
using RegionId = std::string;   // Holds an ASCII literal owned by the container class.

/// @brief One region's declaration. Boundaries are derived by prefix sum, never written by hand.
struct RegionSpec
{
    RegionId      Id;
    std::uint32_t SlotCount = 0;
    bool          bAutoFill = false;
};

/// @brief Derived slot lookup shared by every container; no raw index has to be typed by callers.
class RegionTable
{
public:
    /// @brief Validates that every slot belongs to exactly one region and that the total fits the definition.
    ValidationResult ValidateAgainst(std::uint32_t declaredSlotCount) const;

    /// @brief Absolute slot index of the ordinal-th slot of a region.
    SlotIndex SlotOf(const RegionId& region, std::uint32_t ordinal) const;

    /// @brief Region that owns the slot, empty name when the index is out of range.
    RegionId RegionOf(SlotIndex slot) const;

private:
    std::vector<RegionSpec> m_Specs;
    std::vector<std::uint32_t> m_Begin;
};
```

区域 id 用稳定 ASCII 名字而不是 GameplayTag：tag 词汇表是**内容作者可见的语义词汇**（C37/C38），区域是容器实现的布局细节；把布局塞进词汇表会让 tag 命名空间被实现细节污染，并且废弃一个区域会连带触发 C41 的保留义务。什么样的 tag 才进词汇表：物品的准入 tag（例如「这件物品属于头部装备」），因为那是内容侧要声明的语义。此点列入待决问题 Q2。

#### 4.8.3 典型区域划分

| 容器类型 | 区域（按顺序） | 自动填充 | 说明 |
| --- | --- | --- | --- |
| 玩家背包 | hotbar、storage、equipment（每部位 1 槽）、offhand、craft input、craft output | hotbar + storage | 装备/副手/合成/产物只能显式放置；产物位只读 |
| 箱子 | storage | storage | 无区域语义也可以：只有一个区域，声明表长度为 1 |
| 熔炉 | input、output、fuel | 无（全部显式） | 三个单槽区域，语义全在区域上 |
| 工作台 | material 0..8、product | 无 | product 只读；关闭时材料交回背包或转溢出 |
| 网格容器（箱子变体） | storage | storage | 槽位与网格格一对一（第 4.9 节） |

### 4.9 几何与槽位解耦

#### 4.9.1 三条边界

1. **占位形状属物品。** `Footprint{ 宽, 高 }` 与「允许的朝向集合」由物品实例的片段视图给出（定义装配的片段 + 实例覆盖列表合并后的结果）。基类与容器都不持有形状字段。
2. **二维网格属派生类。** 只有 `GridInventory` 及其派生持有网格；基类连「网格存在」这件事都不知道。这保证线性容器（玩家背包、装备位）不为网格付任何概念成本。
3. **朝向属放置记录，不属物品。** 同一把剑横放还是竖放取决于放它的那一次操作，不是物品自身的状态。若把朝向写进物品实例增量，会产生两个后果：物品在容器间搬运时携带无意义状态；`CanStack` 判据被迫考虑它，于是两个朝向不同的同种物品不再合并。因此朝向存在网格容器的放置表里，物品实例只有定义引用、数量、增量、覆盖列表。

（示意，不是最终签名）

```cpp
/// @brief Quarter-turn orientation of a placed footprint.
enum class PlacementRotation : std::uint8_t { Deg0, Deg90, Deg180, Deg270 };

/// @brief Shape an item occupies on a grid, plus the orientations its definition allows.
struct Footprint
{
    std::uint32_t Width  = 1;
    std::uint32_t Height = 1;
};

/// @brief Grid container: adds a second coordinate system on top of slot indices.
class GridInventory : public InventoryBase
{
public:
    /// @brief Grid cell a slot maps to; together with CellToSlot it must be a bijection.
    virtual GridCell SlotToCell(SlotIndex slot) const = 0;
    /// @brief Slot a grid cell maps to; out-of-grid cells report an invalid slot.
    virtual SlotIndex CellToSlot(GridCell cell) const = 0;

    /// @brief Tries to place an item starting at an anchor cell, honouring rotation legality.
    virtual InventoryOpResult TryPlaceAt(GridCell anchor, PlacementRotation rotation,
                                        const InstanceRef& item, std::uint32_t count) = 0;

protected:
    // The grid is a derived-class member, not a base-class obligation.
    std::uint32_t m_Columns = 0;
    std::uint32_t m_Rows    = 0;
};
```

#### 4.9.2 一槽一格

**决定：网格容器的槽位与网格格一对一。** 一个槽位就是一个格，因此「一件 2×2 的物品」在槽位层面是「4 个槽位的占用状态指向同一件物品」，而不是「一个槽位跨 4 格」。

这个选择让整块设计简化：

- 槽位记录的写入单位仍然是一个槽位，事务（第 4.4 节）完全不需要跨槽位回滚；
- 「哪些格被占」是派生量，可由「锚点槽 + 形状 + 朝向」重算，不必写盘（C11）；
- 放置 = 一组槽位的写入 + 一条放置记录，移除 = 同一组槽位清空，两者都还在原语义内；
- 双射（`SlotToCell` / `CellToSlot`）可以断言，作为放置算法的护栏。

代价是槽位数量等于格子数量（大网格 = 大量槽位），可接受：槽位记录是值类型，且访问是按下标。

#### 4.9.3 线性容器与网格容器差异表

| 维度 | 线性容器（玩家背包 / 箱子 / 熔炉） | 网格容器（2×2 或更大的箱子变体） |
| --- | --- | --- |
| 槽位与坐标 | 槽位索引即全部坐标 | 槽位索引 ↔ 网格格双射，两套坐标 |
| 基类 | `InventoryBase` | `GridInventory`（额外成员：列数、行数、放置表） |
| 占位形状是否参与判定 | 不参与；一个槽位放一件（一堆） | 参与；放置需要形状与朝向 |
| 放置入口 | `TryAddTo(slot, ...)` | `TryPlaceAt(anchor, rotation, ...)` + 基类入口视为「自动寻位」 |
| 放置失败原因 | 容量 / 占用 / 准入 | 以上 + `FootprintNoFit` |
| 派生量 | 空槽数、是否满 | 以上 + 占用格图、剩余连续空块 |
| 序列化额外字段 | 无 | 每个占用槽组的朝向（放置记录） |
| 扫描顺序 | 由区域顺序决定 | 由派生类的扫描顺序决定（第 4.9.4 节） |
| 整理能力 | 天然无碎片 | 有碎片，且第一版不做整理 |

#### 4.9.4 放置是 2D bin packing，接受启发式

多格物品的放置是二维装箱问题的实例，最优解是 NP-难。**本 spec 明确接受启发式**：不承诺「能放下就一定放下」，只承诺「放下或给出明确的失败码」。

由派生类决定的策略点（必须显式声明，不得散落在算法里）：

1. **扫描顺序**：行优先、列优先、还是区域优先。它决定「同样的空位下先试哪个锚点」，直接影响玩家感到的「自动放置直觉」。
2. **旋转合法性**：是否允许 90° / 270°，哪些物品可旋转（由物品的片段视图给允许朝向集合），旋转是否参与扫描（先试 Deg0 再试旋转，还是先试所有锚点再试旋转）。
3. **碎片化处理**：不处理（首选）、移位压缩（第一版不做）、拒绝并要求玩家手动整理。第一版选「不处理」，理由与第 2.2 节的「不做整理」一致：压缩会重映射槽位索引，是丢物品的高发路径。
4. **失败语义**：必须返回 `FootprintNoFit`，且结果里带上「尝试过的锚点数」用于诊断（不作为对外承诺）。

### 4.10 堆叠、上限与拆分

#### 4.10.1 CanStack 判据

**规则：两槽可合并，当且仅当以下三项同时成立。**

1. `ContentId` 相等（同一份定义）。
2. 实例增量表相等：tag 键集合相同，且每个键的值相等（比较语义，不比较插入顺序；序列化时按 tag 名升序输出，见第 6.3 节）。
3. 两件的**覆盖列表都为空**。

**为什么不用「属性全等」。** 属性全等有两个致命问题：

- **当前值是派生的。** 属性当前值包含修饰器合成结果（C6）。同一份定义的两堆石头，在玩家带着「幸运」效果时当前值可能不同，但它们在堆叠语义上完全等价。用当前值比较会让「玩家有 buff 时背包不合并」，且这个行为随效果生灭而变化，无法测试、无法解释。
- **基值比较是恒真式。** 基值由定义给出（C4/C6），同定义必然相同，比较它没有任何信息量，只是把「有没有真正比较实例状态」这件事掩盖掉。

**为什么覆盖列表必须为空而不是相等。** 覆盖列表表达的是「这一个实例被特别改过」，它本身就是唯一性标记。允许合并会让拆分产生两个带相同覆盖的实例——行为上似乎等价，但实例身份重复；一旦将来引入实例唯一 id 用于引用，就会变成两件同 id 的物品，那是 dupe 的制度化版本。因此：**带覆盖列表的实例视为唯一实例，不可合并、不可拆分**（第 4.10.3 节）。

#### 4.10.2 MaxStack 的来源

**规则：`MaxStack` 的唯一来源是物品定义侧的堆叠片段；实例增量不得写它。**

为什么不是实例属性：

1. 堆叠上限决定「一个槽位能放几个」，容量策略要用它算余量。若它能按实例变化，容量计算就必须逐件比较「以哪一件为准」，而合并结果又依赖遍历顺序——顺序依赖是 C27 明令禁止的形态。
2. 若上限来自实例增量，每件堆叠物都要多写一条增量，直接冲淡 C14 的「省略与显式为零可区分」——大部分物品根本不需要这条键。
3. 「容器层面的堆叠加成」（例如「材料堆叠上限 +10」）同样不写在每件物品上：它是容器定义里的整数字段或一条显式变更入口，且**不做成属性**（C48：上限属整型语义的量）；它只在装载期或显式入口落地，不在每次堆叠判定时重算。

可检验：把 `MaxStack` 写进某个实例的增量表，装载期报问题项；`MaxStack` 缺省（片段缺失）时按降级规则视为 1（第 7.1 节）。

#### 4.10.3 拆分契约

`TrySplit(from, count, target, to)`：

| 项 | 规则 |
| --- | --- |
| 前置 | `count > 0`；`count <` 源槽数量（拆全部等价于移动，调用方用 `TryMove`）；源槽非空；源实例覆盖列表为空 |
| 目标校验 | 目标槽必须为空或与源可合并且合并后不超上限；必须通过目标容器的准入与容量；不得是同一个槽位 |
| 写入 | 源槽数量减 `count`；目标槽写入同一 `ContentId` 的**深拷贝**实例状态，数量为 `count` |
| 原子性 | 两处写入在同一计划内；任一校验失败则都不写（第 4.4 节） |
| 拒绝条件 | `CountInvalid` / `NotEnough` / `EmptySource` / `Occupied` / `NotAdmitted` / `CapacityExceeded` / `SameSlot` 幂等成功 / `ReadOnlySlot` / `OverrideInstance` |
| 通知 | 提交后按 (容器, 通道) 发一次，去抖合并（C28）；槽位级差异由接收方回查代数（第 4.11 节） |

拆全部为什么不允许：`count ==` 源数量时若走拆分路径，会同时出现「源清空」与「目标新增」两种写法，与移动路径重复；保留单一路径可以让「同一动作只有一种写法」，从而只有一处可能出错。

### 4.11 通知与去抖（细化 C28）

通道与载荷：事件只做冒泡通知，不带业务数据（C24）。做法是**每 (容器, 通道) 一条 tag 通道** + 容器侧的**变更代数**：

- 去抖键固定为 (对象, 通道) = (容器, 通道)，框架级不可再细（D2 裁定）；持有者可以声明更细的子键，但同一 (容器, 通道) 至少合并一次。
- 容器默认只在一个「槽位变更」通道上冒泡。**区域或条目级的差异一律通过回查变更代数表达，不得靠多条通知**：通知里不带区域字段、不带槽位字段，也不按区域分通道。
- 容器维护每个槽位的 `SlotGeneration`（单调计数，仅运行期存在）。提交阶段只递增被写槽位的代数，不发送事件。
- 帧末 drain 时，对每个脏通道发一次通知（发送者 = 容器）。
- 接收方收到通知后回查容器：读 `SlotGeneration` 找出哪些槽位的代数变了（包括它关心的区域），再读 `Query(slot)` 得到终态。这就是「单一真值 + 回查」而不是「载荷传输」。

去抖规则的落地（C28）：同一帧内同一容器同一通道的多次变更合并为一次通知，即使这些变更落在不同区域或不同槽位；去重只丢弃重复的**通知**，不丢弃最后一次**状态**——状态早就在槽位里，通知只是唤醒信号。**不得**用「一个区域一条通知」表达区域差异：那会让通知条数变成业务信息，并把区域的增删写进通知契约。

两条禁止：

1. 通知处理中不得再改容器（C29 单一写入点）。需要「拾取后自动放入下一级容器」这类链式行为，必须写成显式状态机在 Variable 相位推进（C27），不得串成事件链。
2. 广播期间不得增删订阅（C26）；订阅句柄一律 RAII，容器销毁前先让句柄失效（C25/C34）。

### 4.12 溢出分工

三种可选策略与归属：

| 策略 | 归属 | 说明 |
| --- | --- | --- |
| 拒绝 | 容器 | 默认形态：结构化结果里 `Rejected > 0`，物品仍归调用方。`TryAdd` 天然如此 |
| 交给执行器 | 持有者 | 容器产出 `OverflowRequest{ 物品集合, 原因, 表现参数 }`；掉落实体、抛散算法、位置全在执行方 |
| 转入父容器 | 调用方策略 | 例如「关闭工作台时材料先回背包，装不下的再溢出」；这是两步调用，不进容器 |

**决定：「转入父容器」不内建。** 容器不知道自己被谁持有（C31 的真值归属表把持有者放在会话/场景层），内建父子链会让容器持有另一个容器的引用，形成双向依赖与生命周期耦合；而且「父容器满后再怎么办」的策略会级联，最终把一整棵策略树塞进基类。

**为什么背包不自己生成世界掉落物。** 三条理由：

1. **依赖方向。** 容器在核心层与适配层，世界实体在场景层；容器 include 世界实体头会形成「世界 → 容器 → 世界」的反向依赖（总纲 §4.2 第 1/2 条）。
2. **信息不足。** 掉落需要位置、朝向、抛散半径、是否落在持有者脚下。容器没有位置，也不该有位置——位置属于被销毁的方块或死亡的角色，属场景侧。
3. **可测试性。** 容器只发请求，就可以在没有任何场景的 doctest 里验证「装不下时交出了什么」；一旦容器内部生成实体，容器用例就必须启动整个世界。

采纳既往实现的 `OnOverflow` 分工：容器声明「要交出去什么」与「表现参数（半径、向上冲量、是否落在持有者脚下）」，执行方决定「怎么生成」。

### 4.13 生命周期与时序

| 阶段 | 发生什么 | 约束 |
| --- | --- | --- |
| 装载闸口 | 容器定义装载、区域边界按参数计算并校验、准入 tag 全部校验、网格尺寸校验 | 全部失败为 fail-fast + 完整问题列表（C18/C19）；tick 内不得发生（C23） |
| 创建 | 持有者构造容器实例（派生类型按定义选择）、绑定策略与过滤器、建立空槽位 | 不在构造函数里获取其他子系统（C33） |
| 运行期 | 变更入口只在 Variable 相位被调用；提交后置脏；帧末 drain 发通知 | Fixed 相位调用变更入口直接失败并断言（C22） |
| 快照 | 持有者在装载闸口或显式请求路径上序列化容器 | 写盘不得在 tick 内（C16/C23） |
| 销毁 | 先让订阅句柄失效，再把剩余物品作为溢出请求交出（若策略如此），最后释放槽位 | 关闭期不广播、不获取其他子系统（C34） |

### 4.14 边界情况总表

| 情形 | 行为 |
| --- | --- |
| 空容器加 0 | `Rejected` + `CountInvalid`（不视为成功） |
| 容器满时加一批能拆成多堆的物品 | 部分接受，余量如实返回；不发溢出请求（拒绝策略） |
| 同一容器内 Move 到自身 | 幂等 `Ok`，无写入、无通知 |
| Move 到目标容器同一槽 | 同上（跨容器但同一槽位概念不成立，判定依据是「同一容器且同一索引」） |
| Swap 同一槽 | 幂等 `Ok` |
| 容器扩容后旧槽位 | 索引不变，内容不变，代数不变（不伪造变更通知） |
| 显式上限下调后已超容 | 不裁剪、不删槽位；容器进入只出不进（第 4.15 节） |
| 已超容时移除 | 允许，直到低于上限；每次移除后重新判定是否解除只出不进 |
| 网格容器移除多格物品 | 清空该组槽位 + 删除放置记录；占用格图重算 |
| 网格放置越界 | `Rejected` + `FootprintNoFit`（越界与碰撞用同一原因，另附诊断计数） |
| 只读区域被 `TryAddTo` | `Rejected` + `ReadOnlySlot` |
| 反序列化时某槽索引 ≥ SlotCount | 整份文档拒绝装载（问题项带文档定位），不部分装载 |
| 反序列化时物品 ContentId 缺失 | 收集全部缺失 id 后整份拒绝（C13），不静默清空槽位 |

### 4.15 已超容（OverCap）状态

容量上限是容器定义里的整数字段，运行期只能经显式容量变更入口改变（第 4.6.2 节的扩容与一次性上限变更）。一次显式上限下调之后，容器可能已经「超额」——已持有物品数量超过当前上限；此时槽位仍在、物品不动，只是不再接受增加。**容量不从属性派生**：属性只有 `float`（C48），逐次读属性当前值算容量会让上限随修饰器生灭而漂移，且没有任何一次「容量变了」的操作可供对账与通知，`OverCap` 也就没有可判定的进入时刻。

**规则：不裁剪。** 不销毁、不自动丢弃、不自动转溢出。容器进入 `OverCap` 状态：

- 所有增加操作返回 `Rejected` + `CapacityExceeded`；
- 移除操作照常允许；
- 每次移除后重新判定；数量降到上限以内即退出 `OverCap`；
- `OverCap` 是派生量（由槽位总量与策略上限算出），不写盘（C11）。

理由：自动裁剪会静默销毁玩家资产，这是最不可接受的失败形态；而「只出不进」的语义是自洽的、可解释的、可测试的。容量为负或零的容器定义属装载期 fail-fast（C17 例子中已列「背包容量为负」）。

## 5 契约（K1..K26）

每条都是「规则 + 可检验」，并标注它细化了总纲的哪条契约。

**K1 所有变更入口返回结构化结果。（细化 C21）**
规则：每个公开变更入口返回 `InventoryOpResult`（整体 code / 聚合 reason / accepted / rejected / slot / `Entries` 逐条目结果），不存在语义为「是否成功」的公开布尔函数；`TryQuery` 是唯一的只读试算入口。整体 `Code` 可返回 `PartiallyAccepted`，但每个被拒条目必须各自携带 `RejectReason`；单条操作的结果里 `Entries` 恰好一项，且该条目只有 `Ok` / `Rejected` 两档，不升为第三档（C21 的裁定粒度）。
可检验：静态检查——公开函数里返回类型为布尔且语义为「是否成功」的数目为零；审查每个变更入口的返回类型；一条批量部分接受的用例断言 `Entries` 逐条给出被拒槽位与原因、聚合 accepted/rejected 等于逐条之和；一条用例断言忽略结果时调试期触发断言。

**K2 拒收原因完备且互斥。（细化 C21、C17）**
规则：任何 `Code != Ok` 的结果都必须填一个非 `None` 的 `RejectReason`，取值来自第 7.3 节的封闭清单；批量部分接受按条目填——每个被拒条目至少一条原因，聚合 `Reason` 只作摘要、不得替代条目原因；新增原因必须同时新增用例。
可检验：一条穷举用例遍历原因清单并对每一项构造触发场景，断言返回的原因精确匹配；`RejectReason::None` 只与 `Ok` 同时出现（断言）；一条批量用例断言「逐条目原因齐全」且「聚合原因存在时条目原因仍在」，即聚合不掩盖逐条信息。

**K3 数量与边界规则固定。（细化 C21）**
规则：`count == 0` 拒绝为 `CountInvalid`；有符号入参为负先拒绝；`TryAdd` 超过堆叠上限时允许部分接受，`TryAddTo` 单槽裁剪到上限；越界索引为 `OutOfRange`；源槽空为 `EmptySource`；移除超过持有量为 `NotEnough` 且整次拒绝；源 == 目标为幂等 `Ok` 且不产生写入。
可检验：第 4.5 节表格每一行至少一条用例。

**K4 变更是两段式计划-提交。（细化 C29、C21）**
规则：每个变更入口内部为「构建计划 → 全量校验 → 提交」；校验失败不应用任何一项；提交阶段不失败、不分配、不回调、不查注册表、不发通知；禁止边遍历边改。
可检验：审查项——变更入口的提交流程中不出现资产加载、对象创建、事件派发、注册表查询；一条用例构造「多写入项中第二项校验失败」的场景，断言第一项也没有被应用。

**K5 不存在补偿路径（无回滚）。（细化 C21、C29）**
规则：代码里不存在「先改后补偿」的写路径；若需要调整，只允许调整计划再提交一次；不允许为回滚新增公开入口。
可检验：grep 检查——变更实现里不出现 `Undo` / `Rollback` / `Restore` 语义的函数名与注释；审查所有槽位写入点都落在提交函数内。

**K6 调试期守恒断言。（细化 C17 第二级）**
规则：每条变更在调试期校验「操作前后各定义总量之差 = 本次声明的进出量」；不等即断言并打印定义 id 与差值。
可检验：一条故障注入用例（人为在提交里漏写一项）在断言关闭配置下仍返回失败结果码或产生可检测的不一致；断言开启时触发断言并给出可读消息。

**K7 槽位唯一持有，跨容器一律值拷贝。（细化 C36、C17）**
规则：一个物品实例同一时刻只被一个槽位持有；容器内不保存共享实例引用；跨容器搬运把实例状态深拷贝进目标槽位；禁止把实例内部字段的地址长期持有。
可检验：审查项——`SlotRecord` 无指针成员；一条用例做跨容器移动后断言源槽已空；一条双槽持有检测用例在调试期触发断言。

**K8 通知在提交之后，按 (容器, 通道) 去抖。（细化 C28、C22、C29）**
规则：去抖键固定为 (容器, 通道)；同一帧内同一 (容器, 通道) 的多条变更至少合并为一次通知，即使它们落在不同区域或不同槽位；通知在提交完成后由帧末 drain 发出；通知不带业务数据，也不带区域或槽位字段；区域或条目级差异一律由接收方回查变更代数（哪些槽位代数变了）表达，不得靠多条通知；通知处理中不得再改容器。
可检验：一帧内在同一容器的两个不同区域各改一次，接收方只收到一次通知，且回查代数能分辨出两处都变（C28 在背包侧的特化）；同一通道连续改三次仍只收到一次通知并读到终态；一条用例在通知回调里调用变更入口，断言调试期触发断言。

**K9 槽数单调、扩容只追加尾部。（细化 C8、C11）**
规则：容器存活期间 `SlotCount` 只增不减；扩容只在最后一个区域内追加；目标区域不是最后一个区域时拒绝为 `RegionNotExpandable`；没有缩容与自动整理。一次性容量变更只下调可用容量上限，不删除槽位、不重映射索引，因此不构成缩容。
可检验：扩容用例断言旧槽位索引与内容不变、代数不变；对非末区域扩容断言返回 `RegionNotExpandable`；grep 检查无缩容入口。

**K10 容量策略是接口，参数是数据。（细化 C7、C8）**
规则：能否放入、容量余量、能否扩容三问由 `CapacityPolicy` 回答；容量的数值来源只有两处——容器定义里的整数字段（槽数，以及将来的非槽位计量容量）与显式容量变更入口（扩容、一次性变更）；**不得读属性当前值算容量**（C48）；无上限策略不引入任何「无限」哨兵数值参与容量算术。
可检验：三个派生（无上限 / 有上限 / 可扩容）通过同一套契约用例（第 10 节）；审查项——基类不含 `m_Capacity` / `m_MaxSlots` 之类的上限成员（总纲失败模式 9 的检查面）；审查项——容量判定路径上没有属性读取，容量写入只出现在 `TryExpand` 与一次性容量变更入口（C29 单一写入点）。

**K11 准入过滤器与容量正交。（细化 C8、C21）**
规则：准入只回答「允不允许」，容量只回答「还有多少」；两者的拒绝原因不同；准入 tag 全部来自 tag 词汇表（C39），比较用完整名精确匹配。
可检验：两条用例分别制造「只收特定种类」与「容量不足」，断言返回不同原因；未注册的准入 tag 使装载失败（C19）。

**K12 自动填充区显式声明。（细化 C21）**
规则：`TryAdd` 只填充容器声明的自动填充区；`TryAddTo` 忽略自动填充区；装备位、副手位、只读输出位、合成输入位默认不在自动填充区；只读区域拒绝一切增加并返回 `ReadOnlySlot`。
可检验：一条用例在只读区域存在时连续拾取，断言装备位与输出位未被自动填充（既往缺陷的回归用例）。

**K13 区域是代码常量且边界由单一声明表推导。（细化 C8、C44）**
规则：区域以有序 `RegionSpec` 表声明，边界由前缀和推导，禁止手写 begin/end 常量；区域 id 用稳定 ASCII 名字；取用通过 `SlotOf(region, ordinal)` 并做范围断言。
可检验：审查项——派生类里不出现 `kXxxBegin` 形式的区域边界常量；一条静态用例断言 `SlotOf(region, 0)` 与 `SlotOf(region, count-1)` 落在期望区间。

**K14 区域边界在装载期校验。（细化 C17、C18）**
规则：由数据参数算出的区域边界若越界、重叠、或与定义声明的槽数不符，装载期报问题项并终止装载；问题项带容器定义 id 与区域 id。
可检验：构造「定义声明 20 槽、区域表合计 21 槽」的定义，装载失败且问题项定位到具体区域。

**K15 几何不是基类义务。（细化 C6、C11）**
规则：基类与 `SlotRecord` 不含任何几何字段；占位形状与允许朝向来自物品实例的片段视图；二维网格与放置表是网格容器的成员。
可检验：审查项——基类头文件里不出现 `Width` / `Height` / `Rotation` / `Grid` 语义的字段；一条用例用同一份容器基类接口跑线性容器，不引入任何网格概念。

**K16 一槽一格且映射为双射。（细化 C11）**
规则：网格容器的 `SlotToCell` / `CellToSlot` 必须是双射，槽数等于格数；一件多格物品的占用表现为「一组槽位指向同一件物品」。
可检验：一条用例遍历全部槽位与格，断言往返一致；断言 `SlotCount == 行数 × 列数`。

**K17 朝向属放置记录，不属物品实例。（细化 C11、C14）**
规则：`PlacementRotation` 存在网格容器的放置表里，不写进物品实例的 tag 增量；`CanStack` 判据不含朝向；序列化必须包含朝向（否则往返丢失）。
可检验：两条用例——同一物品以不同朝向放置后搬运到线性容器，断言搬运后无朝向残留；序列化往返后朝向一致。

**K18 放置失败必给明确原因。（细化 C21）**
规则：放置算法允许启发式，但失败必须返回 `FootprintNoFit`；扫描顺序、旋转合法性、碎片化策略由派生类显式声明，不得隐含在算法实现里。
可检验：一条用例在只剩 1×1 空洞时放置 2×2 物品，断言返回 `FootprintNoFit` 且容器状态未变；审查项——三个策略点各有一处显式声明点（虚函数或构造参数）。

**K19 CanStack 三段判据。（细化 C5、C6）**
规则：可合并 = `ContentId` 相等 + 实例增量表键值集合相等 + 两件覆盖列表都为空；禁止用属性当前值或基值做判据。
可检验：三条用例——不同定义不合并；同定义同增量合并；同定义但一件带覆盖列表不合并。另有一条对偶用例：玩家带修饰器效果时同种物品仍合并（证明未用当前值比较）。

**K20 MaxStack 唯一来源是定义侧片段。（细化 C6、C7）**
规则：堆叠上限只由物品定义侧片段给出；实例增量写入该键在装载期被拒绝；片段缺失时按降级规则视为 1（第 7.1 节）。
可检验：写一条含 `MaxStack` 键的实例增量，断言装载期问题项；缺片段用例断言上限为 1 且产生一条降级警告，降级开关关闭时变失败。

**K21 拆分契约固定。（细化 C21、C29）**
规则：`count` 必须严格小于源槽数量；目标必须为空或可合并且合并后不超上限；两处写入在同一计划内；带覆盖列表的实例不可拆分；拆全部不允许（用 `TryMove`）。
可检验：第 4.10.3 节表格每行一条用例，含「目标容器拒收时源槽数量未变」这一条（原子性证据）。

**K22 溢出只发请求，容器不认识世界。（细化 C24、C31）**
规则：容器最多产出 `OverflowRequest`（物品集合 + 原因 + 表现参数）；容器不生成实体、不持位置、不 include 世界层头文件；「转入父容器」是调用方的两步调用。
可检验：grep 检查——容器头文件的 include 集合不含世界/实体层头文件；一条用例在无场景环境下断言装不下时交出的物品集合与原因正确。

**K23 序列化只存最小集。（细化 C11、C9）**
规则：容器持久化文档只含 `schemaVersion`、容器定义引用、槽位记录数组（槽位索引 + `ContentId` + 数量 + 实例增量表 + 覆盖列表）、网格容器的放置朝向；不存派生量（占用格图、是否满、空槽数、属性当前值、`OverCap`）；槽位记录与物品实例都是反射派生对象，不是内联结构体。
可检验：遍历文档结构断言字段集合与清单一致；一条用例填充容器后序列化，断言文档里不出现 current / occupied / isFull 语义的字段名。

**K24 往返等价。（细化 C11、C14）**
规则：同一容器 save → load → save 两次字节序列等价；槽位按索引升序输出，增量表按 tag 名升序输出，覆盖列表按 (片段种类, 动作) 排序；「省略」与「显式为零」必须可区分。
可检验：一条往返用例做两次序列化并逐字节比较；一条用例写入值为零的增量后往返，条目数与值均保持（C14 的背包侧特化）。

**K25 文档版本闸。（细化 C12）**
规则：容器文档带整型 `schemaVersion`，当前只有 1 合法；其它值整份拒绝并报不支持版本；不写空迁移函数。
可检验：把版本改成 0 与 2，各自装载失败并给出可读原因。

**K26 引用解析两段且失败整份拒绝。（细化 C13）**
规则：反序列化第一段只还原结构与 `ContentId`；第二段在引擎完成对象引用解析后按 id 查注册表；缺失 id 全部收集成问题列表；失败时整份文档拒绝装载，不静默清空槽位、不部分装载。
可检验：构造一份含两个不存在 id 的文档，装载返回至少两条问题项且容器未被构造；审查项——解析路径里没有「置空继续」的分支。

## 6 数据与序列化

### 6.1 进 DataAsset 的东西（容器定义）

| 字段 | 形态 | 说明 |
| --- | --- | --- |
| 内容 id | `MGFContentIdRef`（CORE-F03 提供的 kind + name 两字段） | C1；不进资产路径；本 spec 不自造任何其他 id 编码 |
| 容器派生类型 | 类名字符串（白名单内） | 由反射工厂实例化派生容器；白名单防止内容指定任意类 |
| 槽数参数 | 整型数组（按区域顺序对应区域的槽数） | 容量就在这里：整数字段，不做成属性（C48）；区域 id 是代码常量，参数只是每段多少个；装载期校验总数与派生类预期一致 |
| 网格尺寸 | 两个整型（列、行） | 仅网格容器使用；`SlotCount` 必须等于乘积（K16） |
| 准入 tag 集合 | 字符串数组（tag 完整名） | C39/C40；装载期逐个校验已注册（C19） |
| 自动填充区声明 | 字符串数组（区域 id） | 必须是该派生类声明过的区域 id，否则装载失败 |
| 堆叠上限缺省值 | 整型 | 仅用于「物品片段缺失」的降级路径（第 7.1 节），不覆盖物品自身片段 |
| 溢出表现参数 | 若干浮点与布尔（半径、向上冲量、是否落在持有者脚下） | 容器不含位置；参数只描述表现 |
| 文档 schemaVersion | 整型 | 属持久化文档，不属容器定义；列在此处仅为提醒不要混用 |

受 C9 约束，以上字段全部是 primitive / 对象引用 / 数组，禁止内联结构体；「槽数参数」与「自动填充区」用并行数组 + 顺序约定，不用嵌套结构。

### 6.2 派生量（不写盘）

- 空槽数、是否满、`OverCap` 状态、容量余量；
- 网格占用格图、连续空块、放置可行性；
- 物品的属性当前值（由 CORE-F02 重聚合，C6）；
- 槽位的变更代数（运行期唤醒信号，非状态）；
- 区域边界（由代码表 + 数据参数算出，装载期校验后即为常量）。

### 6.3 持久化最小集的精确形状

（示意，不是最终签名；实际由反射对象图承载）

```text
ContainerDocument
  schemaVersion : int                   // 1 only; anything else is refused
  containerKind : string                // reflected class name, checked against the allow list
  definition    : MGFContentIdRef        // content id reference of the container definition
  slots         : SlotRecordDocument[]  // sorted by slot index ascending
SlotRecordDocument
  slotIndex     : uint                  // must be below SlotCount of the resolved definition
  item          : MGFContentIdRef        // content id reference of the held item; omitted when empty
  count         : uint                  // above zero; omitted when empty
  tags          : { name, value }[]     // instance increment table, sorted by tag name
  overrides     : { fragment, action, params }[]   // explicit override list, sorted
  rotation      : uint8                 // grid containers only; omitted for linear containers
```

「持有物 id + 数量 + 占位信息」的最小集就是上表的 `item`、`count`、`rotation` 三项（外加身份 `slotIndex` 与实例增量/覆盖列表）。占位**形状**不在其中——它由物品定义侧的片段给出，物品换了定义就换了形状，写进存档就是第二处真值。

往返等价如何保证：三条机制合起来覆盖全部不确定来源——(1) 槽位按索引升序、增量表按 tag 名升序、覆盖列表按 (片段种类, 动作) 排序；(2) 省略与显式零用「条目存在性」区分（C14），不用哨兵值；(3) 派生量一律不入存档（C11），因此不存在「载入时算出来与存盘时不同」的字段。可检验的判据就是 K24 的字节比较。

## 7 校验与失败

### 7.1 三级分类在本 spec 的落点

| 级别 | 本 spec 的具体落点 |
| --- | --- |
| 加载期 fail-fast | 容器定义的槽数参数为负或与派生类预期不符；区域边界越界或重叠；准入 tag 未注册；自动填充区引用了未声明的区域；网格尺寸非正或与槽数不符；`MaxStack` 非正；文档 `schemaVersion` 非 1；文档槽位索引 ≥ `SlotCount`；同一槽位索引在文档中出现两次；文档里的物品 `ContentId` 未注册；文档朝向值不在枚举内 |
| 运行期断言 | 数量为负或零却仍占着槽位；同一实例被两个槽位持有；区域覆盖出现空洞或重叠（运行期重算后比对）；计划在提交阶段校验失败（说明校验被绕过）；Fixed 相位调用变更入口；守恒断言不成立；`SlotCount` 减少 |
| 允许的降级 | 物品定义缺少堆叠片段时按上限 1 处理并记一条警告；缺少几何片段时按 1×1 处理并记一条警告。降级开关关闭后两者升为 fail-fast（C17 第三级的强制要求） |

### 7.2 问题种类清单（问题项 kind）

问题项的结构沿用总纲（kind + 定位 + 细节文本，C18）。本 spec 的 kind 封闭清单：

| kind | 级别 | 定位 |
| --- | --- | --- |
| `Inventory.Definition.InvalidSlotCount` | fail-fast | 容器定义 id + 区域 id |
| `Inventory.Definition.RegionOutOfRange` | fail-fast | 容器定义 id + 区域 id |
| `Inventory.Definition.RegionOverlap` | fail-fast | 容器定义 id + 相邻两个区域 id |
| `Inventory.Definition.UnknownAutoFillRegion` | fail-fast | 容器定义 id + 区域 id 字符串 |
| `Inventory.Definition.UnknownAdmissionTag` | fail-fast | 容器定义 id + tag 完整名（C19 要求消息里给出需要新增的词汇条目） |
| `Inventory.Definition.InvalidGridExtent` | fail-fast | 容器定义 id + 列/行值 |
| `Inventory.Definition.InvalidMaxStack` | fail-fast | 物品定义 id + 片段 |
| `Inventory.Document.UnknownSchemaVersion` | fail-fast | 文档路径 + 读到的版本 |
| `Inventory.Document.SlotOutOfRange` | fail-fast | 文档路径 + 槽位索引 |
| `Inventory.Document.DuplicateSlotIndex` | fail-fast | 文档路径 + 槽位索引 |
| `Inventory.Document.MissingContentId` | fail-fast | 文档路径 + 槽位索引 + 缺失 id（全部收集，C13） |
| `Inventory.Document.InvalidRotation` | fail-fast | 文档路径 + 槽位索引 + 读到的值 |
| `Inventory.Item.MissingStackFragment` | 降级（可升为 fail-fast） | 物品定义 id |
| `Inventory.Item.MissingGeometryFragment` | 降级（可升为 fail-fast） | 物品定义 id |

运行期不产生问题项，只产生结果码 + 断言（第 4.3 节的结果结构），因为运行期失败对调用方是「这次操作不行」，不是「内容要修」。

### 7.3 拒收原因清单（结果里的 RejectReason）

下表是**逐条目**原因的取值域：批量结果里每个被拒条目各填一个值，聚合 `Reason` 只是摘要（第 4.3 节）。

| 值 | 含义 |
| --- | --- |
| `None` | 仅与 `Ok` 同时出现 |
| `OutOfRange` | 槽位索引越界 |
| `CountInvalid` | 数量为零或为负 |
| `EmptySource` | 源槽为空 |
| `NotEnough` | 源槽数量不足以移除请求量（整次拒绝） |
| `Occupied` | 目标槽被占用且不可合并 |
| `NotStackable` | 该物品不可堆叠（上限为 1）却要求合并/拆分 |
| `CapacityExceeded` | 容器或槽位已达上限（含 `OverCap` 状态） |
| `NotAdmitted` | 准入过滤器拒绝（种类或槽位维度） |
| `ReadOnlySlot` | 目标区域只读 |
| `FootprintNoFit` | 网格放置失败（越界或碰撞） |
| `RegionNotExpandable` | 扩容目标区域不是最后一个区域 |
| `OverrideInstance` | 目标或源是带覆盖列表的唯一实例，不可合并或拆分 |

## 8 可迁移性注记

逐条判断：去掉 C++ 与 minEngine API 之后是否仍成立。

| 条目 | 判断 | 条件或说明 |
| --- | --- | --- |
| 槽位身份就是下标，不额外包槽位对象 | 成立 | 任何语言都能用一段连续记录表达 |
| 区域是代码常量 + 单一声明表 + 前缀和推导边界 | 成立 | 与语言无关；换成表格驱动的语言时，表即数据但仍是代码内常量 |
| 容量策略与准入过滤器是抽象接口 | 成立 | 面向接口的策略对象在任何语言里都存在；没有接口的语言用函数指针/闭包表达，语义不变 |
| 无上限策略不使用哨兵数值 | 成立 | 这是「不要用 magic number 表达规则」的通用判据 |
| 容量是定义里的整数字段 + 显式变更入口，不从属性派生 | 成立 | C48 的通用判据：整型语义的量不做成浮点属性；容量变更必须是可对账、可通知的一次显式操作 |
| 槽数单调、扩容只追加尾部 | 成立 | 索引稳定性是持久化身份的前提，与引擎无关 |
| 结构化结果 + 封闭拒收原因清单 + 无布尔成功 | 成立 | 诊断模型与语言无关 |
| 两段式计划-提交、提交不失败、无补偿路径 | 成立 | 本 spec 最值得迁移的一条：它把「事务」还原成「先算后写」这一个约束 |
| 守恒断言作为 dupe 探测器 | 成立 | 只依赖数量会计 |
| 一槽一格 + 槽位/格双射 | 成立 | 几何退化为槽位写入的前提 |
| 朝向属放置记录而非物品状态 | 成立 | 概念归属问题，与引擎无关 |
| 多格放置是 NP-难，接受启发式 | 成立 | 但「可接受」是产品判断：若某款游戏要求「能放下必须放下」，需要换精确算法或限制形状集合，契约（失败原因 `FootprintNoFit`）不变 |
| 按 (容器, 通道) 去抖通知 + 变更代数回查 | 条件成立 | 需要「帧末 drain」概念；没有帧的宿主用「显式 flush 点」，语义不变；区域或槽位级差异靠回查代数，不靠多条通道 |
| 溢出只发请求、执行方在场景侧 | 条件成立 | 需要一条「请求可被其他层接收」的通道；若目标引擎没有事件总线，用回调注册同样成立 |
| 容器持有二维网格作为派生类成员 | 成立 | 派生 + 成员，与引擎无关 |
| 用反射派生类承载容器与槽位记录 | 条件成立 | 需要反射或多态对象；没有反射的语言用普通类层次，契约不变 |
| 槽位记录与物品实例必须是反射派生对象而非内联结构体 | 不成立 | 这是本宿主反射属性无 inline struct 的具体限制（C9）；迁移时应按目标引擎能力重新决定嵌套形态，「单一权威来源」的意图仍成立 |
| 用 tag 名字数组表达准入集合 | 条件成立 | 需要字符串键词汇表；没有 tag 系统时用自建词汇表模块，规则不变 |
| 序列化用反射 archive + 整型 schemaVersion | 条件成立 | 需要某种序列化基础设施；契约（最小集 + 确定性顺序 + 拒绝未知版本）不变 |

结论：不成立项只有「反射无 inline struct 导致嵌套形态」一条，且属宿主限制而非设计意图；其余可原样写进另一种语言。

## 9 与其它块的关系

### 9.1 依赖方向与真值归属

| 对方 | 方向 | 谁持有真值 | 本 spec 需要对方提供 | 本 spec 提供给对方 |
| --- | --- | --- | --- | --- |
| CORE-F01（总纲） | 本 spec 依赖它 | 契约文本 | C5/C21/C28/C29/C31 等条文 | 无（向下游只读） |
| CORE-F03（内容定义） | 本 spec 依赖它 | 定义与 `MGFContentIdRef` 注册表、实例类 | 按 `MGFContentIdRef` 查定义；读取定义装配的片段；构造实例；提供实例类与实例的有效片段视图（定义装配 + 覆盖列表合并结果，已裁定由 F03 提供，见 Q5） | 容器定义需要的字段形态（第 6.1 节，受 C9 约束）；容器与槽位只用 `MGFContentIdRef`，不新增 id 编码 |
| CORE-F02（属性与效果） | 只读 | 属性当前值与效果 | 无容量相关诉求：容量不来自属性（C48），背包不读容器属性，属性生灭不改变容器容量 | 承诺：容量只经显式变更入口改变，属性生灭不触发裁剪（第 4.15 节） |
| CORE-F06（会话与子系统） | 本 spec 依赖它 | 容器实例的持有者 | 容器创建/销毁点、快照点、装载闸口上的重建入口 | 容器的序列化入口与最小集形状（第 6.3 节） |
| 世界掉落执行器 | 单向 | 掉落实体 | 无 | `OverflowRequest`（物品集合 + 原因 + 表现参数） |
| UI / 交互层 | 单向只读 | 无 | 无 | `Query` / `TryQuery` / 变更通知 |

### 9.2 三条必须写清的边界

1. **背包不定义物品。** 物品种类、堆叠上限数值、几何尺寸、准入 tag 全部来自 CORE-F03 的定义与片段的装配。实例类与实例的有效片段视图（定义装配 + 覆盖列表合并结果）由 F03 提供，背包只引用、不自行合并覆盖（D1 裁定；§9.1 的索要项即为「引用其已提供的实例类与视图片」，见 Q5）。
2. **背包不写属性，也不从属性读容量。** 容量是容器定义里的整数字段，只能经显式容量变更入口改变（C48）；属性生灭不改变容量，因此「属性值下降导致超容」这条路径不存在，唯一的降容路径是显式容量变更（第 4.15 节）。背包对 F02 的承诺是：容量判定不依赖属性，属性变更不会触发裁剪。
3. **背包不持有自己。** 容器实例由 CORE-F06 的持有者创建与销毁（C31 表里「背包实例状态：背包实例自身，由所属子系统持有」）；背包不知道自己的持有者，也不知道自己有没有父容器。

### 9.3 与本 spec 相关的反向依赖禁令

容器头文件的 include 集合必须满足三条：不 include 世界/场景层、不 include UI 层、核心层部分不 include 任何引擎头（C46）。这三条都是可 grep 的检查项，已写进 K22 与第 10 节。

## 10 实现波次与验收判据

本 spec 落在总纲 §9 的 **Wave2（Inventory）**，且覆盖 Wave2 的全部设计内容（Wave2 的产物清单与本文档条目一一对应）。前置条件：Wave1 已交付内容注册表、实例形态、问题列表、加载闸口与 doctest/ctest 基线（总纲 §9 Wave1）。

**波内落地顺序与总纲一致：CORE-F06 的相位接入（Variable 相位派发链）是本波首个交付物**（A12(c) 回填），其后依次是背包语义、容量策略、几何与网格。理由：没有相位接入就无法验证「变更不在 Fixed 相位生效」这条判据，本波也就无法独立验收。

### 10.1 交付物与契约条目映射

| Wave2 产物（总纲） | 本文档条目 |
| --- | --- |
| 背包接口语义与结果码 | §4.3、K1、K2、K3 |
| 容量策略接口 | §4.6、K10 |
| 至少三个派生（无上限 / 有上限 / 可扩容） | §4.6.3、K9、K10 |
| 物品实例上的几何占位片段 | §4.9.1、K15、K20 |
| 背包派生类持有的二维网格 | §4.9、K16、K17、K18 |
| 装备/交换的显式操作集合（总纲已注明其数量与边界、拆分语义由本 spec 契约承接：`TryAddTo` 显式区域放置、`TrySwap`、`TrySplit`） | §4.3、§4.5、K3、K21 |

### 10.2 验收判据（全部可执行）

1. **三派生同套用例。** 用同一份契约用例参数化跑三个派生：试加、试加超过上限、部分移除、移除超过持有量、容量不足、越界索引、数量为零、源==目标。全部通过即为「语义一致而策略不同」（总纲 Wave2 判据 1）。
2. **几何与槽位解耦。** 一条用例放置 2×2 物品并断言占用槽位与占用格正确；旋转合法性拒绝返回 `FootprintNoFit`；同一物品以不同朝向搬运到线性容器后无朝向残留；`SlotToCell`/`CellToSlot` 双射断言通过（总纲 Wave2 判据 2）。
3. **相位与去抖。** 先验收相位接入（F06 的首个交付物）：在 Fixed 相位调用变更入口返回失败并触发断言；再验收去抖——一帧内在同一容器的两个不同区域各改一次，只收到一次通知，且回查代数能分辨出两处都变（去抖键为 (容器, 通道)，总纲 Wave2 判据 3）。
4. **序列化最小集与往返。** 填充后 save → load → save 两次字节序列等价；文档字段集合与第 6.3 节清单逐项一致；文档里不出现派生量字段名（总纲 Wave2 判据 4）。
5. **dupe 探测。** 一段随机操作序列（加/移/移/交换/拆分混合）执行后，守恒断言通过且所有容器总量与初始量之差等于显式输入量；另有一条故障注入用例证明断言能捕获漏写。
6. **只出不进。** 经一次性容量变更入口把可用容量上限从 10 降到 3（槽位仍为 10，已有 5 件）：已有物品仍在、槽位索引不变，`TryAdd` 返回 `CapacityExceeded`，`TryRemove` 允许，降到 3 以下后 `TryAdd` 恢复。
7. **问题种类全覆盖。** 第 7.2 节的每个 fail-fast kind 至少一条触发用例；降级两条各有一条用例，并在降级开关关闭时验证升级为失败。
8. **依赖方向。** grep：容器头文件不含世界/场景层与 UI 层 include；核心层部分不含引擎符号（总纲 §4.2 检查项）。
9. **注释与命名。** 源码 ID 与章节号 grep 为空（C43）；公开类型用 `MGF` 前缀（C44）；无超过约 600 行的巨型头文件（C45）。

### 10.3 口径提醒

Wave2 的用例必须能在**没有任何场景**的 doctest 进程里跑通（K22 的直接结果）。任何需要启动世界才能验证的背包行为，都说明层级被写歪了，应回到 §4.12 重新划分。

## 11 待决问题

**Q1 目标槽被占用时 `TryMove` 的语义。**
选项：(a) 拒绝并返回 `Occupied`，交换交给 `TrySwap`（本文档当前写法）；(b) `TryMove` 内建「目标被占用则交换」。
建议 (a)。理由：一个入口只有一种语义时，原子性与通知面都可控；内建交换会让「移动」的结果同时是两种操作，且与 `TrySwap` 的职责重叠。
什么会推翻它：若交互层发现「拖到占用槽」是唯一的高频手势且每次都要先查询再分派，那么 (b) 少一次查询；此时仍需要一个「只移动不交换」的变体入口，(a) 必须是那个变体。

**Q2 区域 id 用 tag 还是稳定字符串。**
选项：(a) 稳定 ASCII 字符串常量（本文档当前写法）；(b) 注册到 GameplayTag 词汇表的 tag。
建议 (a)。理由：区域是容器实现的布局细节，不是内容作者可见的语义；塞进词汇表会污染 tag 命名空间，并让废弃区域触发 C41 的保留义务。
什么会推翻它：若 UI 需要「按区域查询」并且已经有一层通用的 tag 分发（例如所有 UI 通道都按 tag 订阅），那么用 tag 可以省掉一层映射；代价是词汇表多出一族实现细节 tag。

**Q3 自动填充的先后顺序。**
选项：(a) 先合并已有可堆叠槽位，再填空白槽位（本文档当前写法，与既往实现一致）；(b) 先填空白槽位，再合并。
建议 (a)。理由：先合并能减少碎片、减少槽位占用，符合玩家直觉；且 (b) 会在「已有半堆」时先把新物品铺开，导致背包更快满。
什么会推翻它：若某些玩法需要「优先铺开」以便快速分拣（例如自动分拣传送带），那应该在调用方策略里表达，而不是改 `TryAdd` 的默认顺序。

**Q4 网格容器的槽数是否必须等于行数 × 列数。**
选项：(a) 必须相等，双射成立（本文档当前写法）；(b) 允许形状掩码（某些格不可用），此时槽位仍是全部格，但不可用格被准入过滤器永久拒绝。
建议 (a)。理由：掩码会把「不可用」变成运行期规则，放置算法的扫描必须处理它，失败原因也要多一档；(a) 用「网格尺寸」这一个数据参数就够。
什么会推翻它：若出现 L 形箱子或带固定障碍的容器（解谜类常见），(b) 就是必需的；届时掩码只是准入过滤器的一个实现，契约不变。

**Q5 背包读取「实例的片段视图」这一查询由谁提供。（已裁定：由 CORE-F03 提供）**
裁定（D1）：实例类与实例的有效片段视图（定义装配 + 覆盖列表合并结果）由 CORE-F03 提供，本 spec 只引用、不自己实现合并规则，也不自持实例状态定义。
选项：(a) CORE-F03 提供「按实例读出合并后的片段视图」（含覆盖列表生效后的结果，即裁定项）；(b) 背包自己实现覆盖合并规则；(c) 背包直接读定义侧片段，忽略覆盖。
建议 (a)。理由：覆盖合并规则属 CORE-F03 的定义/实例契约，若背包自己实现就是第二处真值（C5/C29 的形态）；(c) 会让「覆盖了占位形状的物品」在网格里形状错。
什么会推翻它：若 F03 认为片段视图是「实例自身的查询方法」而不是注册表服务，那也行——关键是只有一处实现。这是给 CORE-F03 的接口诉求；若 F03 文档已定别的形态，本 spec 跟随 F03。

**Q6 带覆盖列表的实例是否允许拆分。**
选项：(a) 不允许，视为唯一实例（本文档当前写法）；(b) 允许，拆出的副本带同一份覆盖列表。
建议 (a)。理由：拆分会产生两个「被特别改过」的实例，一旦将来引入实例唯一 id 用于引用，就是两件同 id 的物品——dupe 的制度化版本。
什么会推翻它：若出现「同类特殊物品必须能叠放且能拆」的明确玩法需求（例如批量附魔卷轴），则需要给实例引入显式唯一 id，(b) 才安全；那是 CORE-F03 的扩展，不是背包单方面能定的。

**Q7 容器容量是否可以随效果变化。（本版按裁定：不做）**
裁定与本文档做法：容量只来自容器定义里的整数字段（槽数，以及可选的可用容量上限），运行期只经显式容量变更入口改变（扩容、一次性上限变更）；第一版不提供「容量随属性或效果变化」的路径。
理由：(a) C48 规定属性值只有 `float`，槽数与容量属整型语义的量，不得建模成属性；(b) 逐次读属性当前值算容量会让上限随修饰器生灭静默漂移——没有一次「容量变了」的操作可供对账、可供通知，`OverCap` 也就没有可判定的进入时刻；(c) 若改为直接改容器的整数字段来跟随属性，就绕过了唯一写入点（C29），同一份容量出现两处真值。
什么会推翻它：若玩法确实需要「背包格数随某属性增长」，按 C48 补一条**显式取整派生规则**即可——由内容声明取整方向（默认向零）、在派生处写清舍入方向、结果仍是整数字段，并且必须经同一条容量变更入口落地，变成一次可对账、可通知的显式操作；容量判定处永远不现读属性。届时只增加一条实现约束，K10 的契约形状不变。

## 修订记录 R3

按六份设计文档的只读跨文档审查（6 阻塞 / 9 需修 / 5 建议）与作者裁定 D1（实例类归属 CORE-F03）、D2（去抖键 (对象, 通道)）做的一轮定点修订。只改句子、补括注、加一段说明，未重排章节、未改契约编号与章节标题。

| 审查编号 | 改了什么 | 位置 |
| --- | --- | --- |
| B6（阻塞） | 容量模型改为「容器定义整数字段 + 显式容量变更入口」：删除「容量属性当前值驱动」；新增一次性容量变更入口（只调可用上限、不增删槽位），§4.15 的 `OverCap` 触发点由「属性下降」改为「显式上限下调」 | §4.6.1、§4.6.2、§4.6.3、§4.15、§4.14、§10.2 判据 6、K9、K10、§9.1 |
| B7（阻塞） | K8 的去抖键由 (容器, 区域) 改为 (容器, 通道)，并追加「区域/条目级差异靠回查变更代数表达，不得靠多条通知」；可检验项改为「两个不同区域各改一次只收一次通知 + 回查代数可分辨」 | K8、§4.11、§4.4.3 D4、§4.10.3、§8、§10.2 判据 3 |
| B8（需修） | `InventoryOpResult` 改为逐条目携带被拒原因：新增 `EntryResult` 与 `Entries`；单条操作的 `Entries` 恰好一项且只有 Ok/Rejected 两档；K1/K2 与 §7.3 同口径 | §4.3、§3、§7.3、K1、K2 |
| B4（阻塞） | 删除自持且未定义的 `InstanceState`，槽位改持 CORE-F03 提供的实例类 `InstanceContent`，片段视图改引 F03 的 `InstanceContent::GetFragmentView()` / `FragmentView`；§9.2 的索要项改为「引用其已提供的实例类与视图片」，Q5 标记为已裁定 | §4.2.1、§9.1、§9.2、§11 Q5 |
| B15（阻塞） | 资产侧内容 id 引用统一为 CORE-F03 的 `MGFContentIdRef`（kind + name 两字段），删除本 spec 自造的 `{kind, name}` 写法 | §6.1、§6.3、§9.1 |
| A8 | K1/K2 与 C21 的 A8 口径逐字一致：批量可返回部分接受但必须逐条目携带原因，单条操作只有成功/失败两档 | K1、K2 |
| A11 | 全文「容量来自属性」的表述按 C48 一并修正；容量只来自整数字段与显式变更入口，容器层堆叠加成也改出口 | §4.6、§4.10.2、§4.14、§4.15、§8、§9.1、§9.2、§10.2、K10 |
| A12(c) | §10 与总纲 Wave2 判据呼应：注明装备/交换的显式操作集合由本 spec 契约承接，并补「F06 相位接入是本波首个交付物，否则本波无法独立验收」 | §10 开头、§10.1、§10.2 判据 3 |

额外改动与说明（清单外，均为同一裁定的连带修正）：

- 头部依赖行把 CORE-F02 的描述由「属性：容量属性与当前值」改为「属性与效果；容量不经属性」，F03 描述补「实例类与有效片段视图」——否则标题区与 B6 / B4 的正文自相矛盾（位置：文件头 §同级依赖）。
- §3 术语表新增「条目结果 EntryResult」一行，容器定义的括注改为「整数字段：槽数参数即容量」——B8 与 B6 引入的新粒度与新词汇需要在术语表取得身份（位置：§3）。
- §4.10.2 第 3 条原把「容器层面的堆叠上限加成」归入 CORE-F02 的属性域，与 C48 冲突，改为「容器定义整数字段或显式变更入口」（位置：§4.10.2）。
- §4.14 边界表「容量属性下降导致已超容」改为「显式上限下调后已超容」，并补「不删槽位」——触发源变了，行文必须跟着变（位置：§4.14）。
- §7.3 增加一句「该清单是逐条目原因的取值域，聚合 Reason 只是摘要」——B8 之后若仍按聚合口径读这张表，会与 K1 冲突（位置：§7.3）。
- §11 新增 Q7（容量是否随效果变化），记录本版按裁定不做的理由，以及将来若要做的唯一路径（C48 显式取整 + 同一条变更入口）——审查意见与本文档的「派生」叙述有张力，按纪律登记而非擅自偏离（位置：§11）。
- §10.2 判据 6 的构造方式由「改容量属性」改为「经一次性容量变更入口降可用上限（槽位数不变）」——与 B6 同一口径，否则该判据无法执行（位置：§10.2）。
- K9 追加一句「一次性容量变更不删除槽位、不重映射索引，因此不构成缩容」——不写这句，K9 的「槽数只增不减」会与新增的上限下调入口读成矛盾（位置：K9）。

## 12 评审意见（作者填写）

使用说明：

- 作者只填三列：**位置**（章节号或行内引用，例如 `4.4`、`K21`、`第 11 节 Q3`）、**作者意见**（一句话说清问题或不同看法，不必给方案）。
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
