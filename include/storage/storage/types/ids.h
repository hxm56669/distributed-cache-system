#pragma once

#include <compare>
// C++20 三路比较运算符 <=> 相关定义。
// 建议显式包含，虽然某些标准库头可能间接包含它。
#include <string>

namespace storage {
// 定义一个“生成 ID 类型”的宏
#define STORAGE_DEFINE_ID_TYPE(Name)                                      \
    struct Name {                                                         \
        std::string value;                                                \
        [[nodiscard]] bool valid() const noexcept { return !value.empty(); } \
        friend bool operator==(const Name&, const Name&) = default;       \
        friend auto operator<=>(const Name&, const Name&) = default;      \
    }

STORAGE_DEFINE_ID_TYPE(ObjectId);
STORAGE_DEFINE_ID_TYPE(ChunkId);
STORAGE_DEFINE_ID_TYPE(ReplicaId);
STORAGE_DEFINE_ID_TYPE(WorkerId);

#undef STORAGE_DEFINE_ID_TYPE

} // namespace storage
