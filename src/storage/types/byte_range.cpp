#include "storage/storage/types/byte_range.h"

namespace storage {

Status ValidateByteRange(const ByteRange &range, std::uint64_t total_size) {
    // 第一层检查：起始位置是否已经超过整个对象的末尾。
    // 注意这里使用的是 >，而不是 >=。
    //
    // offset == total_size 本身允许通过这一层检查，
    // 因为如果 size == 0：
    //
    // offset = 100
    // size = 0
    //
    // 表示区间 [100, 100)，即空区间，
    // 当前 ValidateByteRange() 的实现认为这种情况合法。
    if (range.offset > total_size) {
        return {StatusCode::kInvalidArgument, "byte range offset exceeds object size"};
    }
    // 第二层检查：从 offset 开始，剩余空间是否足够容纳 size 个字节。
    // 这里故意写成：
    //
    // size > total_size - offset
    //
    // 而不是：
    //
    // offset + size > total_size
    //
    // 原因是 offset 和 size 都是 std::uint64_t，
    // 如果两个非常大的值直接相加，可能发生无符号整数溢出。
    //
    // 例如 offset + size 超过 UINT64_MAX 时，
    // 结果会回绕成一个较小的数字，从而可能错误地通过边界检查。
    //
    // 由于前面的 if 已经保证：
    //
    // offset <= total_size
    //
    // 所以这里计算：
    //
    // total_size - offset
    //
    // 一定不会发生无符号整数下溢。
    //
    // 因此这种写法既完成了：
    //
    // offset + size <= total_size
    //
    // 的逻辑检查，又避免了加法溢出问题。
    if (range.size > total_size - range.offset) {
        return {StatusCode::kInvalidArgument, "byte range size exceeds object boundary"};
    }
    return Status::Ok();
}

} // namespace storage
