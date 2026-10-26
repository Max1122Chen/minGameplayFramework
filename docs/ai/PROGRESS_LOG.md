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

## 2026-10-26（L3 设计调研与框架总纲）

- 范围：CORE-F01（框架分层与跨块契约，总纲）；同时登记 CORE-F02 ~ CORE-F07 与 INFRA-F01 为 Planned。
- 完成：
  - L3 设计调研：minEngine 基建（GameInstance / Subsystem / Tick 时序 / GameplayTag / 事件总线 / DataAsset / 反射 / 序列化 / 模块入口）、minCraft 先例（薄 ItemDef + Fragment 数组 + 双层覆盖、扁平槽位背包、无磁盘存档）、外部经验（Lyra 三段切分、Type Object、图文档、机制/策略分离、事件总线四大坑）、旧项目 CardGameDemo 的小 GAS 教训
  - 确认 13 条设计决策台账（工程形态、实施顺序、测试形态、文档划分、子系统复用、tag 词汇表、实例状态形态、图节点负载、背包边界、小 GAS 范围、内容数据驱动、注释规范、文档语言）
  - 撰写总纲 `docs/ai/CORE/CORE-F01_FRAMEWORK_LAYERING_DESIGN.md`：分层与依赖方向、可迁移边界与判据、47 条跨块契约、三级失败策略、子系统真值归属表、tag 词汇表治理、四波实现与验收判据、12 条待决问题
  - 建立 `docs/ai/CORE/README.md` 域索引
  - 登记 7 个 CORE Feature + 1 个 INFRA Feature，并在领域码表新增 `INFRA`（理由：工程骨架不属于 gameplay 内核）
  - 确立评审意见区约定：标题带编号；文档末尾固定「待决问题」+「评审意见（作者填写）」表；作者只写位置与意见，设计者回填判断依据与处理结论（三选一：接受 / 反驳 + 理由 / 转待决问题）；约定同时写入 `docs/ai/templates/DOC_GOVERNANCE.md` 的 Design Review Comment Area 一节
- 决策要点：
  - 实例状态统一为「Definition 引用 + tag 键增量属性表 + 显式 fragment 覆盖列表」，不再使用枚举键
  - 属性一律 base/current 分离且全量重聚合，不做逆运算
  - 内容只存基值与装配关系，词汇表、片段种类、错误种类、结算规则、schema 版本必须进代码
  - 全局服务一律做成引擎子系统，不造并行 Manager 单例；PIE 隔离要求无静态可变状态
  - 不做存档槽与跨会话迁移，只做实例状态快照的自洽往返（登记为技术债关注点）
- 验证：
  - 文档自检：占位符扫描无命中（模板占位符、未填写日期、上游模板仓档案引用均无新增）、交叉引用指向真实路径、Feature ID 与文件名大小写合规
  - 未执行构建与测试：当前验证基线仍为 docs-only
- 下一步：
  - 等待作者在第 11 节填写评审意见；设计者回填判断依据与处理结论
  - CORE-F02 ~ CORE-F07 按总纲契约逐份撰写；INFRA-F01 待 CORE-F01 评审通过后执行，届时验证基线升级为 tests-required