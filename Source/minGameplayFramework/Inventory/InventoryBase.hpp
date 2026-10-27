#pragma once

#include "MinGameplayFrameworkAPI.hpp"
#include "GameplayContent/InventoryItem/ItemInstance.hpp"

namespace minGameplayFramework::Inventory
{
    /// @brief 背包最薄接口。具体游戏业务差异大，不应做成万能基类；示例族各自扩展。
    /// @note TryAdd/TryRemove：入参堆就地修改，返回实际数量。
    ///       TryAdd：未放入的数量留在 item 上。
    ///       TryRemove：item 入参表示「希望移除的 def + 数量」；返回成功移除量，
    ///       item.stack 变为「仍未移除完的剩余请求量」（与 TryAdd 对称）。
    class IInventoryBase
    {
    public:
        virtual ~IInventoryBase() = default;

        /// @brief 尝试放入；成功数量从 item 的 stack 扣除。
        /// @return 实际接受数量；无法放入则为 0。
        virtual int TryAddItem(InventoryItem::ItemInstance& item) = 0;

        /// @brief 尝试按定义与数量移除。
        /// @return 实际移除数量；未移除完的请求量留在 item.stack。
        virtual int TryRemoveItem(InventoryItem::ItemInstance& item) = 0;
    };
}
