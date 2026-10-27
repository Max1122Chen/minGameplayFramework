# Tech Debt

有意为之的债务必须登记；不要用无登记的过渡代码掩盖问题。

| Debt ID | Status | Area | Summary | Impact | Owner | Opened | Revisit date | Exit condition |
|---------|--------|------|---------|--------|-------|--------|--------------|----------------|
| TD-001 | Open | Inventory/Grid | `TryAutoOrganize` 使用不完备 First-Fit，整理后可能驱逐整理前本可放下的物品 | 调用方若假定整理无损会丢物；当前示例可接受 | Max | 2026-10-27 | 需要无损整理或更强装箱时 | 换成可证明不增驱逐的策略，或整理失败整批回滚，并有回归测试 |

## Status

- `Open`
- `In Progress`
- `Resolved`
- `Rejected`

## 登记要求

- 每条债务都要有可度量的退出条件，不能只写"以后清理"。
- 保留过渡路径时，在同一次变更里登记，说明为什么不能现在收掉。
