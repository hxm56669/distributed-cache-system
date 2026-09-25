#pragma once

#include "storage/common/status.h"
// ValidateByteRange() 需要通过 Status 返回“成功”或“参数非法”等结果。
#include <cstdint>
// 引入固定宽度整数类型。
// 这里需要使用 std::uint64_t 表示大对象中的字节偏移和长度。
namespace storage {
// ByteRange 对应的数学区间为：
// [offset, offset + size)
// 左闭右开，即包含 offset，但不包含 offset + size。
struct ByteRange {
    // 字节区间的起始偏移。
    // 例如 offset = 100，表示从对象的第 100 个字节位置开始
    std::uint64_t offset;
    // 需要访问的字节数量。
    // 例如 size = 20，表示从 offset 开始连续访问 20 个字节。
    std::uint64_t size;
};

// 检查一个 ByteRange 是否落在大小为 total_size 的对象内部。
Status ValidateByteRange(const ByteRange &range, std::uint64_t total_size);

} // namespace storage
