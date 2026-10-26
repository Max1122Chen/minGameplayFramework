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
- 下一步：
  - 起草 CORE 域第一份设计文档（框架分层与通用契约），评审通过后登记 Feature
