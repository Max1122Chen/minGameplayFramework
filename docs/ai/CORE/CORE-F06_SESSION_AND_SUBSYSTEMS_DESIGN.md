# CORE-F06 会话与子系统设计

> 状态：Draft
> 作者：项目作者
> 最后更新：2026-10-26
> 关联契约：C2、C3、C4、C5、C9、C11、C12、C13、C14、C15、C16、C17、C18、C19、C20、C21、C22、C23、C24、C25、C26、C27、C28、C29、C30、C31、C32、C33、C34、C35、C36、C37、C38、C39、C42、C43、C44、C45、C46
> 上游特性：CORE-F01（框架分层与跨块契约，总纲）

## 1 问题与动机

总纲定了「全局服务一律是子系统」「每个状态只有一个真值持有者」「状态变更只在 Variable 相位」，但它没有回答三个落地问题：**谁装配、谁持有、在哪一刻推进**。CORE-F02（属性、修饰器与效果）、CORE-F03（内容定义）、CORE-F04（背包）、CORE-F05（图模型）都需要这三件事，各自回答就会得到四套互不兼容的会话边界。

本 spec 要挡住的四种腐坏：

1. **每个块自建 Manager 单例。** 属性块一个 `AttributeManager::Get()`，内容块一个 `ContentRegistry::Get()`，各自持有一份实例集合。集成时出现双真值（总纲第 7 节失败模式 1 的会话版），且 C30 直接被违反。
2. **tick 顺序靠运气。** 引擎的 Variable 相位会遍历 GI 的 Tickable 子系统，遍历顺序等于 `SubsystemCollection` 的创建顺序；这个顺序由反射发现的枚举顺序决定，不是框架承诺。若「属性结算」与「背包归一」各自是 Tickable 子系统，则它们谁先谁后不确定，帧内结果不可复现。
3. **关闭期崩溃。** 实例仓储在 `Shutdown` 时释放实例，而某个订阅方还挂在这条通知链上；引擎的事件组件 `OnDeactivate` 才整体退订，顺序对不上就是广播到已释放内存（失败模式 5）。
4. **Fixed 相位静默不生效。** 引擎 `GameInstance::OnFixedTick` 的基类实现是空函数，引擎**不会**遍历 GI 子系统去调 `FixedTick`。若派生 GI 忘了派发，需要固定步长推进的东西看起来「有接口、没执行」，且没有任何报错。

### 1.1 依据的引擎事实

以下条目来自参考宿主 minEngine 的代码阅读，是本 spec 全部结论的地基（总纲 §1.1 已把它们列为事实基线，这里只补充本 spec 直接依赖的定位与后果）：

| 事实 | 代码位置（参考宿主） | 对本 spec 的含义 |
| --- | --- | --- |
| `Engine::CreateGameInstance` / `CreateGameInstanceForPIE` 是唯一的创建路径；解析类来自项目设置 `GameInstanceClass`，找不到时**回退到基类** | `Engine.cpp` 的 `CreateGameInstanceOfResolvedClass` / `ResolveGameInstanceClass` | 框架注入自己的 GI 子类靠「项目设置指向子类 + 宿主调工厂」；类名写错不会失败，只会静默退化成基类，所以需要一条启动期自检（K2） |
| `GameInstance::Init` 先初始化子系统集合，再调用派生类的 `OnInit` | `GameInstance.cpp` | 派生 GI 的 `OnInit` 是**唯一**可以安全拿到全部子系统的地方 |
| `GameInstance::Shutdown` 先调 `OnShutdown`，再 `Deinitialize` 子系统集合 | `GameInstance.cpp` | 有顺序要求的释放必须写在 `OnShutdown` 里，不能指望 `Deinitialize` 的调用顺序 |
| `Engine::DestroyGameInstance` = `Shutdown()` + `UnregisterObject` + 释放 | `Engine.cpp` | 关闭序列的边界就在这三个动作之间 |
| `Engine::TickGameplayVariablePhase` 先 `GameInstance::TickSubsystems`，再 `SceneManager::TickVariableGameplay` | `Engine.cpp` | GI 侧永远先于 Scene 侧；同一次 tick 里 GI 的通知早于 Scene 的通知 |
| `GameInstance::TickSubsystems` 用 `ForEachSubsystem` 遍历并只调 `TickableGameInstanceSubsystem::Tick` | `GameInstance.cpp` | 遍历顺序 = 集合内创建顺序，不受框架控制 → 必须收缩成单一驱动器（K5） |
| `Engine::TickGameplayFixedPhase` 每个固定步依次调 `GameInstance::OnFixedTick` 与 `SceneManager::FixedTickGameplay` | `Engine.cpp` | Fixed 相位是一帧内**零到多次**，且 GI 侧显式派发（K11） |
| `TickableGameInstanceSubsystem::FixedTick` 的注释明确「只有派生的 GI 调 `OnFixedTick` 才会被调用」 | `Subsystem.h` | 同上；框架的 Fixed 派发点只有一处 |
| `SubsystemCollectionBase::InitializeDependency<T>()` 带初始化栈 `m_InitializingStack` 用于环检测 | `SubsystemCollection.h` | 惰性获取可以强制创建，不会静默拿空指针（K7） |
| `GameplayEventSystemComponent` 是 Scene 作用域组件，`Scene::GetGameplayEventSystem()` 可能返回空；`Subscribe` 返回 `std::string` listener id，`Unsubscribe(id)`；`OnDeactivate` 清空全部监听 | `Scene.h` / `GameplayEventSystemComponent.h` | 订阅没有句柄语义，必须包装；Scene 上可能根本没有事件组件，订阅要能返回失败（K16、K17） |
| `GameplayEvent` 只有一个 `GameplayTagContainer` 字段 | `GameplayEvent.h` | 引擎事件既无业务字段也无发送者字段；主体定位只能由框架门面补（K18） |
| `SubsystemCollection` 每个集合独立持有 `m_Instances`，并提供测试用的 `RegisterTypeForTest` | `SubsystemCollection.h` | 测试可以构造独立会话，这是拒绝全局单例的硬理由之一（4.9） |

## 2 目标与非目标

### 2.1 目标

- **G1 一个会话骨架。** 明确「我们的 GI 子类 + 一组 GI 子系统」就是会话的全部；不新增第二个会话入口，不新增并行 Manager。
- **G2 一张六类状态的真值归属表。** 细化 C31：每类 gameplay 状态只有一个持有者、一个写入点、一条生命周期。
- **G3 一套可复现的更新模型。** 帧内结算次序硬编码在代码里，不依赖子系统创建顺序；Fixed 相位显式派发；tick 内的禁止操作可用计数器验证。
- **G4 一条事件接入口径。** 引擎 Scene 组件被包成 RAII 句柄，订阅变更在帧末 drain，通知去抖；跨 Scene 与跨会话各自有明确判断。
- **G5 一条启动与关闭序列。** 装载 → 校验 → 重建 → 进入 tick；关闭时先停广播再释放。PIE 隔离与惰性获取写进契约。
- **G6 一条面向未来的存档边界。** 第一版不做存档槽（C15），但把「将来往哪写」写死，避免以后推翻所有权模型。

### 2.2 非目标

- **不做存档槽、自动存档、跨版本存档迁移。** 第一版只保证实例状态快照的自洽往返（C11、C12、C15）；产品化存档登记为技术债。
- **不做编辑器 gameplay 面板。** 编辑器宿主只负责打开、填写、保存 DataAsset（与总纲 §2.2 一致）。
- **不做网络同步，不做 DS/Client 分离。** 引擎 `IsDedicatedServerInstance` 恒为 false，会话假设单进程单客户端。任何引入 DS 的改动都需要新文档。
- **不做资源热重载与运行期内容注册。** 内容包只在加载闸口一次性装载；不提供「重新装载」的产品能力（是否允许切包见 Q3）。
- **不定义属性结算规则、背包容量策略、图推进语义。** 它们分别属 CORE-F02 / CORE-F04 / CORE-F05；本 spec 只提供驱动顺序与所有权。
- **不新增第二套事件系统。** 会话通道是引擎组件语义的同构门面，不是另一套订阅规则（总纲 §8「明确不自建」）。
- **不做性能监视或调试覆盖层。** 帧驱动器只做顺序，不做统计面板。

## 3 术语

本 spec 只新增下列概念；`ContentId`、`TagName`、`Fragment`、`Definition`、`Instance`、问题项、问题列表、`schemaVersion`、RAII 订阅句柄、tick 相位、加载闸口、fail-fast 等共享概念一律指向总纲 §3，不在此另立定义。

| 术语 | 定义 |
| --- | --- |
| 会话 Session | 一份 GameInstance 及其实例化出的全部子系统、内容注册表与实例集合。PIE 下每个 PIE 实例各是一个会话。 |
| 会话骨架 Session skeleton | 框架自己的 GI 子类 + 固定的一组 GI 子系统 + 一个装载器。它是装配根，不是游戏逻辑。 |
| 帧驱动器 Frame driver | 唯一的 Tickable GI 子系统；按硬编码次序推进 Variable 相位的各阶段，并承接 GI 派发下来的 Fixed 步。 |
| 相位闸 Phase guard | 帧驱动器维护的「当前处于哪个相位」的计数与入口自检，供结算入口判定是否可写。 |
| 装载序列 Boot sequence | 装载闸口的实现：装载、校验、重建、就绪或失败的状态机，只在会话启动或显式请求时执行。 |
| 装载报告 Load report | 装载序列的产物：最终状态、定义与片段计数、完整问题列表。装载结束后只读。 |
| 会话通道 Session channel | 框架自建的会话级通知通道，语义与引擎 Scene 通道同构；用于跨 Scene 可见的通知。 |
| 通道作用域 Channel scope | 通道 tag 的域段前缀：`Scene.` 走引擎组件，`Session.` 走会话通道。 |
| 主体定位 Subject locator | 通知里唯一允许携带的定位信息（实例 id 或 `ContentId`），属于指向而非载荷。 |
| 通知键 Notification key | `(主体定位, 通道 tag)`；去抖按此键去重。 |
| 弱绑定 Scene binding | 对 Scene 事件组件的非持有引用：每次使用前向 Scene 重新查询，绝不长期持有组件指针。 |
| 订阅代际 Subscription generation | 总线在关闭或清空时自增的版本号；句柄携带代际，过期句柄不再退订任何东西。 |
| 实例仓储 Instance store | 持有实例所有权的子系统：它拥有实例的**存在**，不拥有实例的**状态字段**。 |

## 4 设计

### 4.1 会话骨架：复用引擎 GI，不替代它

框架的会话骨架是**三个挂点**，全部复用引擎既有机制，不新增会话抽象：

1. **GI 子类**（`MGFGameInstance`，派生自引擎 `GameInstance`）。它覆写三个函数：`OnInit`（装配与装载）、`OnShutdown`（有序关闭）、`OnFixedTick`（显式派发固定步）。它不持有 gameplay 状态，不加反射属性。
2. **一组 GI 子系统**（见 4.2）。全部由引擎的 SubsystemCollection 发现、创建、销毁；框架不写自己的服务容器。
3. **一个装载器**（`MGFContentBootstrapper`）。它是组合根持有的普通对象，唯一职责是执行装载序列；不作为子系统，因为它必须在子系统全部就绪之后才被调用，且自身没有需要引擎托管的状态。

GI 子类与引擎 GI 的关系是**细化而非替换**：Standalone 一份、PIE 每实例一份、创建点由宿主调引擎工厂（C35）。框架不提供第二个会话入口，也不包装工厂。

示意，不是最终签名：

```cpp
// Sketch only; not a final signature.
/// @brief Session hub: runs the load gate and dispatches the fixed phase.
/// @warning Holds no gameplay state; every mutable value lives in a subsystem or an instance.
class MGFGameInstance : public GameInstance
{
public:
    /// @brief Runs the load gate after the subsystem collection is initialised.
    void OnInit() override;

    /// @brief Stops broadcasts first, then releases instances, then the registry.
    void OnShutdown() override;

    /// @brief Dispatches one fixed step; the engine never walks game-instance subsystems for this.
    /// @param fixedDeltaSeconds Step length supplied by the engine fixed clock.
    void OnFixedTick(float fixedDeltaSeconds) override;
};
```

**为什么不在 GI 子类里放业务。** GI 是引擎可见、可被项目设置替换的类；把业务写进去以后，换一份项目设置就换掉整条会话的状态归属。因此 GI 子类只允许出现装配调用、顺序硬编码与派发调用；不允许出现状态字段、遍历游戏对象、内容校验逻辑（K3）。

### 4.2 子系统清单与职责

会话内的 GI 子系统**封闭为五个**（K4）。新增第六个必须先改本文档，理由是子系统清单就是所有权清单，随意扩张会让真值归属表失效。

| 子系统 | 类别 | 生命周期 | 依赖 | Tickable | 持有的东西 | 公开入口（语义） |
| --- | --- | --- | --- | --- | --- | --- |
| `MGFContentRegistrySubsystem` | GameInstance | 随会话，跨 Scene | 无（只依赖核心层规则对象） | 否 | 已装好的定义索引（只读） | 按 `ContentId` 查询定义；导出装载摘要；只给 const 句柄（C4） |
| `MGFInstanceStoreSubsystem` | GameInstance | 随会话，跨 Scene | 内容注册表（惰性） | 否 | 实例的**所有权**：属性实体、物品实例、图实例的句柄表 | 创建、销毁、按 id 解析；枚举只读视图；不暴露实例状态字段的写入口（C29） |
| `MGFSessionEventBusSubsystem` | GameInstance | 随会话，跨 Scene | 无强制依赖；Scene 事件组件按 Scene 惰性绑定 | 否 | 会话通道的监听表、待变更队列、去抖队列 | 订阅（返回 RAII 句柄）、派发、帧末 drain；Scene 通道与会话通道同一门面 |
| `MGFConfigDiagnosticsSubsystem` | GameInstance | 随会话，跨 Scene | 无 | 否 | 只读配置值（内容包清单、降级开关）、装载报告的只读归档 | 读配置；写与读装载报告；降级开关取值 |
| `MGFGameplayFrameDriverSubsystem` | GameInstance | 随会话，跨 Scene | 上面四个（全部惰性） | **是（唯一）** | 相位闸计数、本帧结算游标（无 gameplay 真值） | Variable 相位推进；Fixed 步入口；相位查询 |

三条设计判断：

- **为什么注册表与实例仓储是两个子系统。** 失效模式不同：注册表装载失败会让会话不可用（拒绝启动），实例仓储销毁失败是运行期不变量问题。合在一起以后，「装载失败」就没办法在不影响实例集合的前提下表达；而且装载要整体替换索引（K20），实例仓储从不参与替换。
- **为什么配置与诊断是一个子系统。** 两者都是只读、非 gameplay、装载期写入的数据，共同点是不参与任何结算。拆开只会多一个必须在启动早期被访问的子系统，而启动期访问越少越安全。
- **为什么帧驱动器没有 gameplay 真值。** 一旦它开始缓存「本帧要结算的实例列表」，它就成了第二份真值。它的集合视图必须每帧从仓储重新取（只读视图），结算规则本身由 CORE-F02 / F05 提供。

示意，不是最终签名：

```cpp
// Sketch only; not a final signature.
/// @brief Advances gameplay state in a fixed order once per variable-phase tick.
/// @warning Never loads assets, never serialises, never instantiates reflected types.
class MGFGameplayFrameDriverSubsystem : public TickableGameInstanceSubsystem
{
public:
    /// @brief Runs the variable phase in the documented order.
    /// @param deltaTime Frame delta supplied by the engine.
    void Tick(float deltaTime) override;

    /// @brief Runs one fixed step; reached only through the game instance dispatch.
    /// @param fixedDeltaSeconds Step length from the engine fixed clock.
    void FixedStep(float fixedDeltaSeconds);

    /// @brief True while a settlement entry point may legally mutate state.
    bool IsInSettlementPhase() const;
};
```

### 4.3 真值归属（细化 C31）

C31 的六类状态在本 spec 的落点如下。**持有者指状态的唯一写入点；保管者指持有生命周期与所有权的地方，两者可以是不同对象。**

| # | 状态 | 真值持有者（唯一写入点） | 保管者（所有权） | 生命周期 | 只读访问者 |
| --- | --- | --- | --- | --- | --- |
| 1 | 内容定义与注册表 | `MGFContentRegistrySubsystem`（只在装载序列中写，装载完成后冻结） | 同左（随会话） | 随会话，跨 Scene | 全部 gameplay 块 |
| 2 | 属性与效果实例状态 | 属性实体实例自身（效果施加与移除路径） | `MGFInstanceStoreSubsystem` | 随实例 | 帧驱动器（只读视图）、背包、UI |
| 3 | 背包实例状态 | 背包实例自身（`TryAdd` 与 `TryRemove` 路径） | `MGFInstanceStoreSubsystem` | 随实例 | 装备、交易、UI |
| 4 | 图文档与派生状态 | 图实例自身（迁移路径写文档；派生状态可丢弃，由重算写） | `MGFInstanceStoreSubsystem` | 随实例 | 任务与科技模块、UI |
| 5 | 事件总线 | Scene 的 `GameplayEventSystemComponent`（引擎）与 `MGFSessionEventBusSubsystem`（会话通道） | 引擎（Scene）/ 会话骨架（会话通道） | 随 Scene / 随会话 | 全体（只投递） |
| 6 | 加载问题列表 | 装载器（一次性产物）；装载结束后转 `MGFConfigDiagnosticsSubsystem` 只读归档 | 同左 | 装载期与会话只读 | 宿主、编辑器、UI |

补充一行（非 gameplay 状态，C31 未列，本 spec 新增并显式声明）：

| # | 状态 | 真值持有者 | 生命周期 | 只读访问者 |
| --- | --- | --- | --- | --- |
| 7 | 监听表、待变更队列、去抖队列 | `MGFSessionEventBusSubsystem` | 队列一帧、监听表随会话 | 无（内部） |

三条判据：

- 表中每一行只有一个写入点；出现第二个写入点即为设计错误（K6）。
- 第 2、3、4 行的「保管者不等于持有者」是刻意的：仓储管存在与销毁，实例管自己的字段。仓储对外不提供改属性、改背包、改图的写入口（C29）。
- 第 5 行有两种实现但只有一个语义：两条通道共用同一套句柄类型、同一套 drain 与去抖实现；不存在「会话通道顺序更可靠」这类差异承诺（C27）。

### 4.4 更新模型

#### 4.4.1 两个相位与它们的边界

| 相位 | 引擎调用链 | 每帧次数 | 本框架的职责 |
| --- | --- | --- | --- |
| Fixed | `Engine::TickGameplayFixedPhase` → `GameInstance::OnFixedTick` → `SceneManager::FixedTickGameplay` | 0 到 N（由固定时钟决定） | 只做与固定步长强相关的推进（效果作用时间累积、图计时），且必须由 `MGFGameInstance::OnFixedTick` 显式派发 |
| Variable | `Engine::TickGameplayVariablePhase` → `GameInstance::TickSubsystems` → `SceneManager::TickVariableGameplay` | 1 | 全部状态变更：效果结算、图迁移、背包归一、通知 drain |

**必须写清的事实：引擎不会遍历 GI 子系统去调 `FixedTick`。** 基类 `GameInstance::OnFixedTick` 是空实现，注释也明说派生游戏按自选顺序调子系统 `FixedTick`。因此：

- Fixed 派发点只有 `MGFGameInstance::OnFixedTick` 一处，它调用帧驱动器的 `FixedStep`，再由帧驱动器按硬编码次序调用两个固定步阶段（属性、图）。
- 若派生 GI 忘了派发，需要固定步长的东西不会报错，只表现为「看起来没生效」。所以有一条启动后自检：会话就绪后，若引擎报告固定时钟已启用而在一个固定区间内 `FixedStep` 调用计数为 0，则记一条运行期断言级问题（K11 的可检验项）。

#### 4.4.2 Variable 相位的 tick 顺序

因为引擎按集合创建顺序遍历 Tickable 子系统，**只允许一个 Tickable GI 子系统**（K5）。于是帧内顺序完全由帧驱动器内部的硬编码决定，与创建顺序无关：

| 步 | 阶段 | 内容 | 写谁 |
| --- | --- | --- | --- |
| 1 | 相位进入 | 相位闸置结算中，记录本帧开始时的监听表快照指针 | 只写帧驱动器内部 |
| 2 | 属性 | 效果到期推进、全量重聚合、对账断言（规则见 CORE-F02） | 属性实体 |
| 3 | 图 | 按输入推进迁移、重算可丢弃派生状态（规则见 CORE-F05） | 图实例 |
| 4 | 背包 | 归一与容量、占位一致性校验（规则见 CORE-F04） | 背包实例 |
| 5 | 通知派发 | 按去抖队列派发，使用第 1 步的快照 | 只读真值 |
| 6 | 订阅变更 | 应用本帧排队的订阅与退订（C26） | 监听表 |
| 7 | 相位退出 | 清空队列与去抖，相位闸置空闲 | 帧驱动器内部 |

**为什么次序是属性、图、背包、通知。** 属性是唯一会被效果与图奖励同时驱动的量；图迁移的条件常读属性当前值，所以图在属性之后；背包归一通常由物品被消耗或奖励引起，依赖前两者已结算完；通知永远读最终状态，所以最后（C28 的「去重不丢最后一次状态」直接从这里落地）。

**为什么第 5 步用帧开始时的快照。** 一帧内新订阅的接收方不得收到本帧通知（C26）。先派发、再应用变更，比「先应用再派发但要打标记」少一个状态，也少一类顺序陷阱。

#### 4.4.3 tick 内禁止清单

tick（含 Fixed 与 Variable）内禁止：资产装载、内容包解析、tag 词汇校验、序列化写盘、反射实例化、场景创建与销毁、实例批量创建与销毁、订阅表整体重建（C23）。允许：遍历已知集合、纯算术、读取注册表、入队通知、投递通知。

实现约束（可检验）：帧驱动器维护 tick 深度计数；装载器、序列化入口、反射实例化工厂在入口处读该计数，大于零直接返回失败结果（不是断言，是失败返回，符合 C20 与 C21）。计数不经任何全局变量暴露，而由帧驱动器经惰性获取提供给需要的调用方（K13）。

#### 4.4.4 去抖

- 去抖键为 `(主体定位, 通道 tag)`；同帧同键多次变更只派发一次，派发时读到的永远是结算结束后的最终状态。
- 队列只保留键，不保留值：值在派发时回查，否则缓存就成了第二份真值。
- 去抖逐帧清空，不跨帧累积：本帧未派发的通知不得延后到下一帧，否则「合并」会变成「延迟」，而延迟需要超时语义，不在第一版范围。

### 4.5 事件总线接入

#### 4.5.1 包装目标与引擎现状

引擎的 `GameplayEventSystemComponent` 提供 `Dispatch`、`Subscribe`、`Unsubscribe`，通道是 `GameplayTag`，同步单线程，重入上限 16，`OnDeactivate` 整体清空监听。它缺三样东西，正好是 C25 与 C26 要求的：

1. 订阅只返回一个 `std::string` listener id，没有析构语义，因此需要 RAII 句柄。
2. 广播期间订阅集合是否可变取决于实现细节，不是契约，因此需要排队与帧末 drain。
3. Scene 可能存在但根本没有挂事件组件（`Scene::GetGameplayEventSystem()` 返回空），因此订阅必须能返回结构化失败，而不是空句柄。

#### 4.5.2 门面与句柄

`MGFSessionEventBusSubsystem` 对外只暴露一种句柄类型与一种订阅结果，内部有两个后端（Scene 后端即引擎组件，Session 后端即自建监听表）。两条通道能力对齐：同样的通道 tag 匹配、同样的优先级语义、同样的广播期冻结、帧末 drain 与去抖。

示意，不是最终签名：

```cpp
// Sketch only; not a final signature.
/// @brief Owns one subscription and releases it on destruction.
/// @warning A handle created before a bus reset releases nothing afterwards; check IsValid instead of retrying.
class MGFEventSubscription
{
public:
    ~MGFEventSubscription();

    MGFEventSubscription(const MGFEventSubscription&) = delete;
    MGFEventSubscription& operator=(const MGFEventSubscription&) = delete;

    /// @brief Transfers the subscription; the moved-from handle releases nothing.
    MGFEventSubscription(MGFEventSubscription&& other) noexcept;

    /// @brief True while the owning bus still recognises this listener.
    bool IsValid() const;

private:
    // A weak reference is deliberate: the bus may be destroyed before the subscriber.
    std::weak_ptr<MGFEventBusCore> m_Bus;
    uint64_t m_Generation = 0;
    std::string m_ListenerId;
};
```

三条实现约束：

- 句柄析构时先比较代际；代际不一致只标记失效、不退订（关闭期不退订已清空的表）。
- 同一句柄二次释放触发断言（C17 运行期断言级），发布期返回失败并记录。
- 句柄不可拷贝、可移动；容器里统一按移动语义存放。

#### 4.5.3 广播期间不改订阅集合、帧末 drain

总线维护三个容器：已生效监听表、本帧排队变更、本帧去抖键。派发路径只读已生效监听表；在派发过程中到来的订阅与退订只进排队变更。drain 在帧驱动器的第 5、6 步完成（先派发、后应用变更）。这一点对 Scene 后端同样成立：即便引擎组件允许广播期改表，门面也不允许，因为两条通道行为一致比利用引擎的宽松行为更有价值（C26、C27）。

#### 4.5.4 跨 Scene 与全局事件：判断与边界

| 场景 | 判断 | 理由 |
| --- | --- | --- |
| 同一 Scene 内的通知（组件、对象状态） | 走引擎 Scene 组件的包装 | 复用引擎机制；Scene 生命周期天然对齐 |
| 跨 Scene 可见的通知（切换场景后仍要通知 UI 或存档服务；编辑器 Scene 与 PIE Scene 并存） | 走框架会话通道（自建监听表） | 引擎组件绑 Scene，会话级订阅者会随组件 `OnDeactivate` 全部丢失；把这类订阅绑到 GI 子系统才与「跨场景」的生命周期一致 |
| 跨会话（两个 PIE 实例之间互相通知） | **明确拒绝** | 两个会话各有独立监听表；跨会话通知会让 PIE 隔离失效（C32），这类需求本质是宿主级编排，应由宿主在两个 GI 之外完成 |

**不自建第二套事件系统的边界**（总纲 §8）：会话通道不自带新的订阅语义、新的顺序承诺、新的重入规则；它复用同一套句柄类型、同一套去抖与 drain 实现、同一套通道 tag 词汇规则（`Scene.` 与 `Session.` 前缀，C38）。判据：全仓只有一份去抖实现、一份 drain 实现（grep 计数），新增通道没有新增语义（K19）。

#### 4.5.5 payload 缺失导致的回查模式

引擎事件只有一个 tag 容器，没有业务字段（C24）。本框架的规定：

- 通知承载通道 tag 与主体定位两段，别无一物；主体定位是指向而不是载荷，接收方拿到它之后必须回到真值持有者查询最终状态。
- Scene 后端的引擎事件没有主体字段，因此 Scene 通道只用于**粗通知**（这一类东西变了），接收方要么全量重查自己的关注集合，要么改订会话通道以获得主体粒度。
- 需要「谁触发的」这种信息的流程，**不得**通过事件传递；写成显式状态机加命令式调用（C27）。会话通道的自建表虽有能力加发送者字段，也不加（Q2），以免同一段接收代码换个通道就少了信息。
- 回查失败（主体已被销毁）不是错误：按主体已不存在处理，丢弃该通知并记一条调试日志；不得为了派发通知而延长实例生命周期。

### 4.6 启动与关闭序列

#### 4.6.1 启动

| 步 | 执行者 | 动作 | 失败行为 |
| --- | --- | --- | --- |
| 1 | 宿主 | 引擎 `StartSystems` 完成（对象管理器、tag 管理器等） | 不是本框架职责 |
| 2 | 宿主 | `Engine::CreateGameInstance` 或 `CreateGameInstanceForPIE` | 引擎断言；框架不接管 |
| 3 | 引擎 | `NewObject(GI 子类)`、`SetPIEInstanceId`、`GameInstance::Init` | 类名解析不到时静默回退基类，见第 5 步自检 |
| 4 | 引擎 | SubsystemCollection 发现并初始化全部子系统（顺序未知） | 各子系统只记录集合引用，不做跨子系统访问（K7） |
| 5 | 我们的 GI `OnInit` | 自检：能取到 `MGFGameplayFrameDriverSubsystem`，否则记一条 fail-fast 问题（说明项目设置没指向我们的子类） | 直接进入失败态，不再装载 |
| 6 | 我们的 GI `OnInit` | 读配置（内容包清单、降级开关），执行装载序列 | 见 4.6.3 |
| 7 | 装载序列 | Loading、Validating、Rebuilding、Ready | 任一 fail-fast 问题即 Failed |
| 8 | 引擎 | 之后每帧开始调 `TickSubsystems` 与 `OnFixedTick` | 帧驱动器在非 Ready 状态下直接返回，不推进任何状态 |

装载序列的阶段职责（每阶段只做一件事，便于定位问题）：

| 阶段 | 做什么 | 不做什么 |
| --- | --- | --- |
| Loading | 只读包索引资产，按索引实例化定义资产（反射实例化允许，因不在 tick 内） | 不校验语义 |
| Validating | id 唯一性与合法性、tag 词汇、片段种类、包依赖闭包、词汇表版本、引擎版本闸、图与 payload 白名单 | 不解析引用、不改已有索引 |
| Rebuilding | 两段式引用解析（先结构后 id，C13）、构建查询索引、生成装载报告 | 不写任何 gameplay 实例状态 |
| Ready 或 Failed | 一次性 swap 到注册表，或保留旧索引并记录报告 | 不部分生效 |

#### 4.6.2 关闭

| 步 | 执行者 | 动作 |
| --- | --- | --- |
| 1 | 宿主 | `Engine::DestroyGameInstance`（或 `DestroyGameInstanceForPIE`、`DestroyAllPIEGameInstances`） |
| 2 | 引擎 | `GameInstance::Shutdown`，调我们的 `OnShutdown` |
| 3 | 我们的 GI `OnShutdown` | 置关闭标志（禁止广播、禁止结算、禁止获取子系统）；总线代际自增并清空监听（先停广播）；释放实例仓储中的实例；释放注册表；把装载报告最后一段写入日志 |
| 4 | 引擎 | `SubsystemCollection::Deinitialize`，各子系统 `Deinitialize`（顺序未知） |
| 5 | 引擎 | `UnregisterObject` 并释放 GI |

**为什么有序释放写在 `OnShutdown` 而不是各子系统的 `Deinitialize`。** `Deinitialize` 的调用顺序由集合内部顺序决定，框架不承诺；而先停广播、再释放实例、再释放注册表是有顺序要求的。把有顺序要求的部分放在唯一确定顺序的位置（GI 钩子），`Deinitialize` 里只放释放自己内部资源（C34）。

关闭期三条禁令（K8）：不广播、不新增订阅、不获取其他子系统；违者调试期断言，发布期返回失败并记录。判据是一条开关用例：打开关闭期禁止广播后，关闭路径全绿。

#### 4.6.3 内容包装载失败时的整体行为

框架不猜测、不修补、不半装载。行为分三种，判据明确：

| 情形 | 行为 | 判据 |
| --- | --- | --- |
| 索引缺失或不可读、包依赖不闭包、id 重复或非法、未知片段种类、未知 tag、词汇表版本不足、schemaVersion 不支持、payload 白名单不匹配、图结构非法、会话骨架自检失败 | **拒绝启动**：状态 Failed，注册表保持空（或上一次成功装载的那份），帧驱动器不推进任何状态，会话内不广播任何通知 | 用例：装载失败后一帧内结算计数为 0、属性实体数为 0、无通知派发；装载报告返回全部问题 |
| 内容包为空（0 定义且索引合法） | **降级**：允许以空注册表进入 Ready 并产生一条警告；降级开关关闭时升为拒绝启动（C17） | 用例：开关开则 Ready 且警告一条；开关关则 Failed；两次的问题种类相同、级别不同 |
| 可选片段实现缺失、可选边目标缺失 | **降级**：跳过并警告，其余内容照常进入 Ready | 用例：跳过项在报告里可数，且注册表里其余定义可查 |

补充两条硬判据：

- 失败态的会话必须可诊断、可重试：装载报告在 Failed 后仍可读；重新调用装载序列必须从头执行，不得在 Failed 状态上增量修补。
- 失败态不得触发 gameplay 通知：装载失败不是 gameplay 事件，宿主读报告即可，避免 UI 在半个内容库上重建。

### 4.7 存档边界（与 C15 的接口预留）

C15 明确第一版不做存档槽与跨会话迁移。但「以后要加」会影响今天的所有权写法，所以边界现在写死：

| 规则 | 内容 |
| --- | --- |
| 导出由持有者实现 | 每类真值持有者自己导出：注册表**不导出**（内容来自包，不来自存档）；实例仓储导出实例身份清单；属性实体、背包、图各自导出自己的增量与覆盖（C11） |
| 会话层只编排 | 将来的会话快照服务（第一版不存在）只做三件事：收集各持有者的导出、统一写 `schemaVersion`、原子导入。它不拥有任何 gameplay 数据，不知道字段语义 |
| 导入必须原子 | 导入要么整体替换实例集合，要么整体失败并保留原集合（与 K20 的索引 swap 同构）；不允许导入一半 |
| 不写空实现 | 第一版不留下任何空的导出函数与空迁移函数（C12）；只保留「谁能导出」的所有权结论 |

判据：源码中不存在存档槽与自动存档符号（grep 为空）；本节的四条规则在未来新增存档能力时必须原样满足，否则所有权模型需要重写。这正是现在写下来的目的。

### 4.8 与其它块的接口（C36）

本 spec 是装配根与所有权根。所有跨块接口只传 `ContentId`、实例 id，或带解析状态的引用；不传裸指针作为长期引用。

| 对方块 | 本块提供 | 本块需要对方提供 | 真值归属 |
| --- | --- | --- | --- |
| CORE-F02 属性与效果 | 按 `ContentId` 取定义（const 句柄加失败结果）；创建属性实体；每帧可变相位的属性阶段入口；相位闸查询 | 结算算法、重聚合入口、效果到期推进入口、不变量对账入口 | 属性实体自身 |
| CORE-F03 内容定义 | 装载序列的阶段划分与问题列表聚合；定义查询；包依赖与词汇表版本校验的调用点 | 定义种类表、片段种类枚举、问题种类、序列化 schema 版本常量、定义资产的反射类型 | 注册表（只读） |
| CORE-F04 背包 | 创建物品与背包实例；每帧背包归一阶段入口；实例 id 与内容 id 的解析 | 背包接口语义、容量策略派生、几何占位片段 | 背包实例自身 |
| CORE-F05 图模型 | 创建图实例（文档加可丢弃派生状态）；每帧图阶段入口；两段式引用解析的调用点 | 图文档结构、校验规则、payload 白名单、迁移入口 | 图实例自身 |

反向约束：任何块不得自己创建一个注册表、仓储或总线；不得在自己内部实现第二套 tick 顺序；不得在 tick 内调用装载序列。这三条是审查项（K4、K5、K13）。

### 4.9 为什么不能用一个全局内容注册表单例

单例的诱惑在于「注册表是只读的全局常量」。它不是常量，它是一次装载的**产物**，因此有生命周期、有失败、有隔离要求。五个具体腐坏场景：

1. **PIE 双实例互相污染。** 编辑器里两个 PIE 实例各装载一份内容：A 会话装载包 X，B 会话装载包 X 的新版本（改了数值）。全局注册表只有一个索引槽，后装载者覆盖前者。A 会话的属性基值、片段装配、图 payload 全部来自 B 的内容，症状是 A 的数值随机变化，换一个打开顺序就换一套结果。判据：C32 的双 GI 独立性用例，全局单例必定失败。
2. **测试无法隔离。** doctest 在同一进程内跑全部用例。全局注册表让「装载失败」用例的残留问题列表与半装载状态污染后续用例，用例顺序变成隐藏依赖；也无法为每个用例构造独立的两份注册表来验证隔离。引擎的 `SubsystemCollection` 本来就支持每集合独立实例与测试类型注入（`RegisterTypeForTest`），用单例等于主动放弃这个能力。
3. **关闭期悬垂。** 注册表持有定义对象的强引用。全局单例的析构发生在静态析构期，此时对象管理器与反射系统已关闭，释放派生自引擎对象的定义会访问已销毁的基础设施。子系统由 GI 管理，`OnShutdown` 里确定性释放，不存在这个问题。
4. **装载闸口被绕过。** 全局可访问意味着任何代码在任何时刻都能顺手重新装载；于是在 tick 里装载的成本降到零，C23 只能靠人盯。子系统给出的访问路径是只读查询，写入只在装载序列里，而装载器不是全局可获得的对象，能力上就拿不到写入口。
5. **失败无法表达。** 单例装载要么终止进程，要么留下半装载的中间状态（失败后部分索引已生效）。而框架要求返回完整问题列表（C18）、要求失败态可诊断可重试（4.6.3）、要求要么整体生效要么不生效，这三条都要求装载是一个可以被拒绝的动作，而单例没有「不发生」的形态。

结论：注册表是会话状态（随会话、跨场景、每个 GI 一份），按 C30 的判据它必须是 GameInstance 子系统。这与总纲 §8 「明确不自建 Manager 单例与服务定位器」是同一条约束的两种说法。

## 5 契约

体例为规则加可检验，每条标注它细化或遵守的总纲契约。编号 K1..K23。

**K1 会话骨架只有三个挂点。** 框架的会话由 GI 子类、GI 子系统集合与装载器组成；不新增会话管理器、会话上下文、框架根对象之类的第四种东西。（细化 C30）

可检验：审查项——框架公开类型里不存在语义为全局会话容器的类；全仓自建单例式的 `GetInstance()` 为零。

**K2 唯一创建点是宿主，且类解析失败必须被自检发现。** 框架不包装 `CreateGameInstance` 与 `CreateGameInstanceForPIE`；全仓调用点只有宿主里的两处。由于引擎解析 `GameInstanceClass` 失败时会静默回退基类，会话启动时必须自检帧驱动器子系统可取到。（遵守 C35，补充引擎回退事实）

可检验：grep `CreateGameInstance` 的调用点只有两处；把项目设置的类名改错后启动，装载报告出现一条 fail-fast 问题，而不是静默运行在基类上。

**K3 GI 子类不持有 gameplay 状态。** `MGFGameInstance` 的反射属性集合为空；只允许出现装配调用、顺序硬编码与派发调用。（细化 C31）

可检验：header 工具生成结果中该类的属性数为零；审查项——该类的方法体内不出现遍历游戏对象、内容校验与状态字段。

**K4 会话子系统清单封闭为五个。** 内容注册表、实例仓储、会话事件总线、配置与诊断、帧驱动器。新增第六个必须先改本文档。（细化 C30）

可检验：反射中派生自 `GameInstanceSubsystem` 且属于本框架的类型计数为 5；新增类型未同步文档时评审直接打回。

**K5 全会话只有一个 Tickable GI 子系统。** 帧驱动器是唯一的 `TickableGameInstanceSubsystem` 派生；其他子系统一律非 Tickable，由帧驱动器按硬编码次序调用。（细化 C22、C30）

可检验：反射中 `TickableGameInstanceSubsystem` 派生类计数为 1；用例——帧驱动器内部的阶段次序不随子系统创建顺序变化（打乱创建顺序后逐帧结果一致）。

**K6 六类状态各自唯一持有者。** 4.3 的表是强制的；仓储持有实例所有权但不提供状态写入口。（细化 C31、C29）

可检验：表里每行只有一个写入点；用例——从仓储侧调用改属性、改背包、改图的写入口在调试期断言失败。

**K7 不假设子系统初始化顺序。** 子系统的 `Initialize` 里只记录集合引用，不做跨子系统获取；跨子系统获取在首次使用时惰性完成：先 `GetSubsystem<T>()`，为空再 `InitializeDependency<T>()`（引擎带初始化环检测）。（遵守 C33）

可检验：审查项——`Initialize` 体内不出现其他子系统的获取调用；用例——把子系统初始化顺序打乱（含依赖方先于被依赖方初始化）后全部用例仍通过。

**K8 关闭期不广播、不获取、不订阅。** 有顺序要求的释放全部写在 `MGFGameInstance::OnShutdown`，顺序为禁广播、释放实例、释放注册表；子系统 `Deinitialize` 只释放自身内部资源。（遵守 C34）

可检验：打开关闭期禁止广播标志后关闭路径用例通过；用例——关闭过程中触发一次派发请求返回失败且不回调。

**K9 PIE 隔离与零静态可变状态。** 除 tag 词汇表（编译期常量）外，框架不存在静态或全局可变状态；每个 GI 拥有独立的注册表、实例集合、监听表与队列。（遵守 C32）

可检验：两个 GI 并存时装载计数、实例集合、监听表互不影响；grep 源码中非 const 的 `static` 变量为零（词汇表注册表除外）。

**K10 相位闸是所有结算入口的前置条件。** 结算与变更入口在进入前读相位闸；处于 Fixed 相位或非结算阶段时直接返回失败结果。（细化 C22）

可检验：在 `OnFixedTick` 内调用通用结算入口返回失败并记录一条运行期问题；在帧驱动器第 2 至 4 步内调用成功。

**K11 Fixed 相位由 GI 子类显式派发，且派发点唯一。** `MGFGameInstance::OnFixedTick` 是唯一固定步入口，它调帧驱动器的 `FixedStep`；帧驱动器内部按硬编码次序调用属性与图两个固定步阶段，背包不参与固定步。（遵守 C22，补充引擎不自动遍历的事实）

可检验：grep `FixedTick` 的派发点只有一处；用例——固定时钟启用时若一个固定区间内固定步计数为 0，运行报告出现一条断言级问题。

**K12 帧内结算次序固定为属性、图、背包、通知、订阅变更。** 次序硬编码在帧驱动器里，不得由配置或注册顺序决定。（细化 C22、C28）

可检验：用例——同一帧内同时发生属性变化与背包变化时，通知派发读到的属性已是最终值；审查项——次序常量只出现一处，没有第二处排序逻辑。

**K13 tick 内不做重活，且由计数器兜底。** 装载、序列化、tag 校验、反射实例化、场景与实例批量增删在 tick 深度大于零时直接返回失败。（遵守 C23、C16）

可检验：用例——一帧内反射实例化次数为 0；用例——tick 内调用装载序列返回失败；审查项——tick 调用图不触达装载与序列化 API。

**K14 通知按主体定位与通道去抖，且不丢最终状态。** 同帧同键只派发一次；派发时回查真值，队列里不存值。（细化 C28）

可检验：用例——一帧内对同一对象连续改三次，接收方收到一次通知且读到最终值；用例——两个不同对象同通道各改一次，收到两次。

**K15 帧末 drain 顺序为先派发、后应用订阅变更。** 派发只读本帧开始时的监听快照；本帧内的订阅与退订在派发结束后统一生效。（细化 C26）

可检验：用例——在一次派发过程中订阅同一通道，本次不触发、下一次触发；用例——派发中退订，本次仍收到、下一次不再收到。

**K16 订阅必须是 RAII 句柄，句柄携带代际。** 句柄析构即退订；总线关闭或清空后自增代际，旧句柄只失效、不误退订；二次释放断言。（细化 C25）

可检验：用例——持有者销毁后广播，不崩溃且不再回调；用例——关闭会话后旧句柄析构不访问已释放对象；用例——二次释放触发断言（断言关闭配置下返回失败）。

**K17 Scene 事件组件是弱绑定。** 不长期持有组件指针；每次使用向 Scene 重新查询；Scene 无事件组件时订阅返回结构化失败（含该 Scene 未挂事件系统的问题种类），不返回空句柄、不静默丢弃。（细化 C25、C21）

可检验：用例——在无事件组件的 Scene 上订阅返回失败；用例——场景替换组件后旧句柄失效、新订阅正常工作。

**K18 通知只带通道与主体定位，业务数据一律回查。** 通知结构里不存在数值或字符串业务字段；接收方回查真值持有者。会话通道的粗通知可无主体，带主体定位的通知不得被当作数据源。（细化 C24）

可检验：审查项——通知与事件结构体字段数为 2（通道、主体定位）；用例——发送方改值后接收方回查得到一致结果。

**K19 通道作用域由 tag 域段区分，且只有一套语义实现。** `Scene.` 前缀走引擎组件，`Session.` 前缀走会话通道；两条通道共用同一套句柄类型、同一套 drain 与去抖实现；跨会话（PIE 之间）通知不支持。（细化 C24、C38、C27）

可检验：词汇表注册期校验通道 tag 的域段前缀，非两类前缀报命名问题；grep 去抖实现与 drain 实现各只有一处；框架不提供跨会话通道 API。

**K20 装载序列是原子状态机。** 状态为 Idle、Loading、Validating、Rebuilding、Ready、Failed；Rebuilding 成功后一次性 swap 注册表索引，失败保留上一份；非 Ready 时帧驱动器不推进任何状态。（细化 C21、C23、C13）

可检验：用例——装载失败后注册表与失败前一致（初次装载则为空）；用例——Failed 状态下推进一帧，属性、图、背包状态零变化。

**K21 加载失败行为按三类归类，且降级可关。** 4.6.3 的三行表是强制的：语义性错误拒绝启动；空内容包属于允许的降级（开关的默认取值见 Q5）；可选缺失降级并警告。任何降级都必须能由开关升为 fail-fast。失败态可诊断、可重试、不广播。（细化 C17、C18、C21）

可检验：用例——一份同时含重复 id、未知 tag、未知片段种类的包返回三条及以上问题且状态 Failed；用例——空包在开关打开时 Ready、关闭时 Failed；用例——失败态读取报告可拿到全部问题。

**K22 存档边界：导出由持有者实现，会话层只编排，第一版不落任何空实现。** 注册表不参与存档；导入必须原子。（细化 C11、C12、C15）

可检验：grep 源码中存档槽与自动存档符号为空；审查项——不存在空的导出与迁移函数；新增存档能力时必须引用 4.7 的四条规则。

**K23 跨块接口只传 id 或带解析状态的引用。** 子系统公开接口的参数与返回值不出现裸指针作为长期引用；引用未解析时返回显式 pending 或失败结果。（遵守 C36、C3）

可检验：审查项——公开接口签名里没有裸指针长期引用；用例——引用解析失败时上层收到明确失败结果而不是空指针。

## 6 数据与序列化

### 6.1 字段归属

| 数据 | 载体 | 是否写盘 | 说明 |
| --- | --- | --- | --- |
| 内容包索引（包名、定义资产清单、依赖包、最小词汇表版本） | DataAsset（包索引资产） | 是（资产本体） | 装载序列的唯一输入；不是框架自有文档 |
| 内容定义字段（id、片段装配、静态数值、引用 id） | DataAsset（定义资产） | 是（资产本体） | 由 CORE-F03 定义，本 spec 只消费 |
| 实例增量表、覆盖列表、实例身份 | 运行期内存；将来的快照文档 | 第一版不写盘（C15） | 若未来落盘，遵守 C11 与 C14 |
| 注册表查询索引、实例句柄表 | 内存（派生） | 否 | 由装载与创建路径重建 |
| 装载报告（状态、计数、问题列表） | 内存加引擎日志 | 否 | 诊断数据不进存档 |
| 监听表、待变更队列、去抖队列、相位闸 | 内存 | 否 | 一帧或随会话存活 |
| 图文档与派生状态 | 图实例；派生状态可丢弃 | 文档按 CORE-F05；派生状态不写盘 | 本 spec 只负责持有与驱动 |

### 6.2 往返等价如何保证

- **本 spec 不引入新的持久化文档格式**，因此不新增 `schemaVersion` 语义；C12 的机制留给未来的实例快照。
- 会话层对往返的职责只有两条：不缓存派生值（否则往返后读到旧缓存）、装载与导入原子（失败不半生效）。
- 若将来落盘：输出顺序由持有者决定且必须确定（C11 的两次字节序列等价）；会话层在导出的收集阶段按实例 id 排序，保证跨持有者拼接稳定。

### 6.3 DataAsset 侧限制

沿用总纲 C9：定义资产不使用内联结构体属性。内容包索引因此也被压平成资产路径列表与字符串列表，不嵌套结构体。

## 7 校验与失败

### 7.1 三级分类的落点

| 级别 | 本 spec 的具体落点 | 行为 |
| --- | --- | --- |
| 加载期 fail-fast | 包索引缺失或不可读；包依赖不闭包；`ContentId` 重复或非法；未知片段种类；未注册 tag；词汇表版本不足；内容 schema 与引擎版本不支持；引用解析失败（C13）；payload 类不在白名单；图结构非法（CORE-F05）；背包容量为负（CORE-F04）；GI 类解析为基类（K2 自检） | 装载序列进入 Failed，返回完整问题列表，不进入 tick，不广播 |
| 运行期断言 | 相位闸违反（Fixed 相位调结算入口）；tick 内装载、序列化或反射实例化；句柄二次释放；句柄跨代际退订；非持有者调用状态写入口；关闭期广播；固定时钟启用但固定步计数为 0 | 调试期断言并打印不变量名与上下文；发布期返回失败结果加错误日志，不继续执行非法状态 |
| 允许的降级 | 空内容包（默认行为，可由开关升为 fail-fast）；可选片段实现缺失；可选边目标缺失；主体在通知派发前已销毁（丢弃该通知） | 警告并继续；降级开关可关闭前两类 |

### 7.2 问题种类清单

问题项三要素沿用总纲：种类、定位（哪份定义、哪个片段、哪个字段或索引）、细节文本。

| 种类 | 级别 | 定位粒度 |
| --- | --- | --- |
| `PackIndexMissing` | fail-fast | 包名 |
| `PackIndexUnreadable` | fail-fast | 包名与资产路径 |
| `PackDependencyMissing` | fail-fast | 包名与缺失依赖包名 |
| `ContentIdDuplicate` | fail-fast | 种类、名字、两份来源 |
| `ContentIdMalformed` | fail-fast | 种类与原始字符串 |
| `FragmentKindUnknown` | fail-fast | 定义 id 与片段种类串 |
| `TagUnregistered` | fail-fast | 定义 id、字段、完整 tag 名 |
| `VocabularyVersionTooLow` | fail-fast | 包名、要求版本、当前版本 |
| `ContentSchemaUnsupported` | fail-fast | 资产路径与文档版本 |
| `ReferenceUnresolved` | fail-fast | 定义 id、字段、缺失 id（一次报全部） |
| `PayloadClassNotAllowed` | fail-fast | 图 id、节点 id、类名 |
| `SessionSkeletonMissing` | fail-fast | 期望的子系统类型名（K2 自检） |
| `EmptyContentPack` | 降级（可升为 fail-fast） | 包名 |
| `OptionalFragmentMissing` | 降级 | 定义 id 与片段种类 |
| `OptionalEdgeTargetMissing` | 降级 | 图 id、边索引、目标节点 id |
| `SceneEventSystemAbsent` | 运行期失败返回 | Scene 标识（订阅方在失败结果里拿到） |
| `PhaseViolation` | 运行期断言 | 入口名与当前相位 |
| `HeavyWorkInTick` | 运行期断言 | 入口名与 tick 深度 |
| `SubscriptionDoubleRelease` | 运行期断言 | listener id |
| `WriteByNonOwner` | 运行期断言 | 状态类别与调用方标识 |
| `BroadcastDuringShutdown` | 运行期断言 | 通道 tag |
| `FixedStepNotDispatched` | 运行期断言 | 会话标识与已过固定区间数 |

### 7.3 结果类型

装载序列与订阅入口都返回结构化结果（成功、问题列表、失败种类），不使用布尔加全局标志（C21）。语义：

- 装载序列返回装载报告：最终状态、定义数、片段数、问题列表。
- 订阅返回句柄或失败原因（含种类与 Scene 标识）。
- 阶段入口（属性、图、背包）返回执行了、被相位闸拒绝、内部失败三者之一。

示意，不是最终签名：

```cpp
// Sketch only; not a final signature.
/// @brief Outcome of one load sequence; read-only once produced.
struct MGFLoadReport
{
    MGFSessionLoadState FinalState = MGFSessionLoadState::Idle;
    uint32_t DefinitionCount = 0;
    uint32_t FragmentCount = 0;
    std::vector<MGFProblem> Problems;
};
```

## 8 可迁移性注记

去掉 C++ 与 minEngine 具体 API 之后逐条判断。

| 条目 | 判断 | 说明与条件 |
| --- | --- | --- |
| 会话骨架等于引擎会话对象子类加一组作用域服务加一个装载器（4.1） | 条件成立 | 需要目标引擎提供会话对象与作用域服务容器。没有时用一个显式生命周期的服务容器实现，所有权规则不变 |
| 子系统清单封闭、单一 Tickable 驱动器（K4、K5） | 成立 | 帧内顺序不能依赖服务注册顺序，这条与语言无关；任何有服务容器的运行时都会遇到这个坑 |
| 六类状态唯一持有者，且保管者与持有者分离（4.3、K6） | 成立 | 所有权与写入点分离是通用设计约束 |
| 相位闸与显式派发固定步（K10、K11） | 条件成立 | 需要固定步与可变步分离的运行时。若无固定步，保留加载与结算分离这一半即可 |
| 帧内结算次序硬编码（K12） | 成立 | 与引擎无关，是可复现性的最低成本手段 |
| tick 内禁止重活并用深度计数器兜底（K13） | 成立 | 计数器是可移植的兜底手段 |
| 事件门面、RAII 句柄、帧末 drain、去抖（K14 至 K17） | 条件成立 | 需要按通道订阅且可退订的事件设施。目标引擎若有强类型事件总线，可省掉 tag 通道，但无 payload、单一真值、不依赖顺序、帧末生效四条必须保留 |
| 场景作用域通道与会话作用域通道并存（4.5.4、K19） | 条件成立 | 需要引擎提供场景级事件作用域。若目标引擎只有全局事件，会话通道即成为唯一通道，规则不变 |
| 弱绑定场景组件与句柄代际（K16、K17） | 条件成立 | 需要能查询组件是否还活着。没有时退化为销毁前显式退订加关闭期断言，语义不变但错误更晚暴露 |
| 装载序列状态机与原子 swap（K20） | 成立 | 与引擎无关；失败不留半状态是通用要求 |
| 三级失败策略与问题种类清单（7.1、7.2） | 成立 | 诊断模型与语言无关；种类名可换，级别划分不变 |
| PIE 隔离与零静态可变状态（K9） | 成立 | 多实例编辑是现代引擎共性，无 PIE 时也无害 |
| 存档边界：导出由持有者实现、会话层只编排（K22） | 成立 | 这条正是为了防止将来加存档时推翻所有权模型 |
| 引擎类解析失败静默回退基类（K2 自检的由来） | 不成立（作为约束） | 这是参考宿主的具体行为；迁移时应把「会话类必须被验证为期望类型」重做，但装配入口必须自检的意图成立 |

结论：不成立项只有一条，且它是宿主类解析行为的具体缺陷而非设计意图；其余条目在另一种语言与引擎上可原样落地。

## 9 与其它块的关系

### 9.1 依赖方向

- 本 spec 位于装配层：依赖核心层规则值与引擎适配层设施（子系统、事件、反射），**不依赖**任何 gameplay 块的具体规则。
- CORE-F02、F03、F04、F05 依赖本 spec 提供的查询、创建与阶段入口；本 spec 不反向依赖它们的实现，只在组合根装配期接上。
- 任何块都不得绕过组合根自行注册子系统或装载内容包（总纲第 4.2 节的禁止项 4）。

### 9.2 谁持有真值

一句话版本：**内容只读在注册表；状态在实例；实例的存在在仓储；顺序在帧驱动器；通知在总线；问题在诊断。** 出现某个块自己也存了一份，即为双真值。详见 4.3 的表。

### 9.3 需要对方提供的接口（摘要）

| 对方块 | 必须提供 | 必须接受 |
| --- | --- | --- |
| CORE-F02 | 可变相位属性阶段入口、固定步属性阶段入口、重聚合与对账入口、创建属性实体的工厂 | 相位闸查询（结算入口在 Fixed 相位被拒）；只通过实例 id 与 `ContentId` 交互 |
| CORE-F03 | 定义种类与片段种类枚举、问题种类、schema 版本常量、定义资产反射类型、两段式解析所需的 id 字段 | 装载序列的阶段划分；校验一律返回问题列表而不是终止进程 |
| CORE-F04 | 背包接口语义、容量策略派生、几何占位片段、归一入口 | 帧内次序（在属性与图之后）；Fixed 相位不参与 |
| CORE-F05 | 图文档结构与校验、payload 白名单、迁移入口、派生状态重算入口 | 两段式引用解析；派生状态不写盘 |

## 10 实现波次与验收判据

本 spec 落在总纲第 9 节的 **Wave1**（GI 注入、注册表、属性），并在 Wave2 与 Wave3 扩展驱动器：

- Wave1 交付：GI 子类（含自检与 Fixed 派发骨架）、五个子系统骨架、装载序列状态机、问题列表与诊断归档、帧驱动器的阶段次序骨架、Scene 通道的 RAII 包装、相位闸。
- Wave2 交付：帧驱动器的背包阶段接入；去抖与 drain 在背包通知上的用例。
- Wave3 交付：帧驱动器的图阶段接入；固定步图阶段；payload 解析经 C13 接入装载序列。
- 会话通道（跨 Scene）在 Wave1 建骨架，Wave2 起有真实订阅者。

验收判据（全部可执行）：

1. `ctest` 全绿，且至少覆盖 K1、K2、K5、K6、K7、K8、K9、K10、K11、K13、K14、K15、K16、K20、K21 各一条用例。
2. 用例 `session reports a problem when the configured game instance class is not the framework one` 通过（K2）。
3. 检查项：反射中本框架的 `GameInstanceSubsystem` 派生类型计数为 5，`TickableGameInstanceSubsystem` 派生类型计数为 1（K4、K5）。
4. 用例 `subsystem initialisation order does not change frame results` 通过（K7）。
5. 用例 `shutdown broadcasts are refused and no callback fires` 通过（K8）。
6. 用例 `two sessions keep independent registries and listeners` 通过（K9）。
7. 用例 `no reflected object is created inside a tick` 与 `load entry point fails inside a tick` 通过（K13）。
8. 用例 `three edits in one frame deliver one notification with the final value` 通过（K14）。
9. 用例 `subscribing during a broadcast fires on the next broadcast` 通过（K15）。
10. 用例 `load failure keeps the previous registry and advances no state` 通过（K20）。
11. grep 检查：`CreateGameInstance` 调用点两处；去抖实现与 drain 实现各一处（K2、K19）。
12. 源码注释与测试名的 ID 与章节号 grep 为空（C43）。

## 11 待决问题

每条给出选项、建议与什么会推翻建议。建议不是决定。

**Q1 是否允许第二个 Tickable GI 子系统（例如性能监视或调试覆盖层）。** 选项：(a) 禁止，任何 Tickable 需求都并入帧驱动器；(b) 允许注册只读观察者类 Tickable 子系统，但不许写 gameplay 状态。
建议 (a)。理由：一旦允许第二个 Tickable，帧内顺序又回到取决于创建顺序，而这道题的正确成本是零，观察者可以挂在帧驱动器的阶段回调上。
什么会推翻它：若出现必须独立于结算阶段的逐帧任务（例如网络心跳、后台任务泵），且它明确不触碰 gameplay 状态，可为 (b) 开一个白名单并写进本文档。

**Q2 会话通道是否携带发送者信息（Scene 通道受引擎限制不能带）。** 选项：(a) 两条通道都不带发送者，只带通道与主体定位；(b) 会话通道额外带发送者标识，Scene 通道不带。
建议 (a)。理由：两条通道能力对齐后，接收代码可以换通道而不丢信息；谁触发的属于编排，应写成显式状态机与命令式调用（C27）。
什么会推翻它：若实践中大量接收方需要谁改的来做权限或回显，且用状态机表达的代价明显更高，转 (b) 并在文档里显式标注能力差异。

**Q3 加载闸口是否允许运行期重新装载内容包（切包）。** 选项：(a) 第一版只允许会话启动时装载一次；(b) 允许在显式请求路径上整包替换（原子 swap）。
建议 (a)。理由：YAGNI；重载会立刻引入重载期间实例引用了旧定义的一致性问题，而 Wave1 到 Wave3 都不需要它；K20 的原子 swap 已为 (b) 留好形态。
什么会推翻它：若示例内容需要不改进程换内容来做校对，则把 (b) 纳入 Wave4 并补一条重载后实例引用如何失效的规则。

**Q4 Fixed 相位的派发范围。** 选项：(a) 只派发属性与图两个阶段；(b) 四个阶段全派发（含背包与通知）。
建议 (a)。理由：背包是离散操作集合，不需要固定步；通知必须每帧一次，固定步里发通知会让一帧多次固定步变成一帧多次通知，直接与去抖语义冲突。
什么会推翻它：若出现必须在固定步内结算的背包行为（例如按固定步推进的装配进度），把该行为提升为独立阶段再讨论。

**Q5 空内容包降级的默认开关。** 选项：(a) 默认关闭，空包即 fail-fast；(b) 默认打开，允许启动空会话。
建议 (a)。理由：默认值应该让忘记配内容包暴露为启动失败；空会话的价值主要在编辑器与测试里，这两处可以显式打开开关。
什么会推翻它：若编辑器宿主的默认工作流就是先起空会话再填内容，默认应改为打开，并在文档里写清这是编辑器便利而非运行期语义。

**Q6 诊断数据是否自己落日志文件。** 选项：(a) 只进内存报告与引擎日志；(b) 框架自己写一份问题清单文件。
建议 (a)。理由：日志通道是宿主职责，框架再开一条会形成两个地方都要看的排查负担；报告已结构化，宿主想落盘可自行做。
什么会推翻它：若宿主没有可用的日志落盘能力，或需要把问题清单交给非工程角色阅读，则 (b) 有实际价值。

**Q7 实例 id 的生成方式。** 选项：(a) 会话内递增整数，导入时由文档提供并做冲突检查；(b) 内容 id 加会话内序号；(c) 全局唯一随机 id。
建议 (a)。理由：最少的机制即可满足会话内唯一与可落盘，且冲突检查让导入的失败显式化。
什么会推翻它：若将来需要跨会话合并存档（多个存档片段合并），(c) 才值得引入。

**Q8 相位闸在 Fixed 相位是否允许只读入口。** 选项：(a) 相位闸只拦写入口，读入口任意相位可用；(b) 读入口也在 Fixed 相位被限制。
建议 (a)。理由：Fixed 相位本来就允许读当前值做固定步推进；拦读会逼出为固定步复制一份快照的反模式。
什么会推翻它：若出现读入口隐式触发重聚合的情况（即读有副作用），则必须收窄为 (b)，并在 CORE-F02 里把重聚合改成显式入口。

## 12 评审意见（作者填写）

使用说明：

- 作者只填三列：**位置**（章节号或行内引用，例如 `4.4.2`、`K12`、`第 11 节 Q3`）、**作者意见**（一句话说清问题或不同看法，不必给方案）。
- **判断依据**与**处理**两列由设计者回填：判断依据写为什么当初这么定（契约条文、引擎事实、外部经验、代价权衡）；处理结论三选一：**接受** / **反驳加理由** / **转待决问题 Qn**。
- 一行一条意见；不同意就直接写，不需要组织语言。
- 处理完成后，作者只需复核反驳与转待决两类行。

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
