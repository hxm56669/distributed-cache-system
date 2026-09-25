#pragma once

#include "storage/common/status.h"

#include <string>

namespace storage {

// 校验和算法类型。
// 当前项目只支持 BLAKE3，后续如果需要可以继续增加 SHA256 等算法。
enum class ChecksumAlgorithm { kBlake3 };

// 表示一份数据的校验和。
// 可以用于 Object 或 Chunk 的完整性校验。
struct Checksum {
    ChecksumAlgorithm algorithm;   // 记录当前 value 使用哪一种哈希算法
    std::string value;             // 保存哈希值，目前约定为小写十六进制字符串

    // 自动生成 Checksum 的相等比较运算符。
    // 会依次比较 algorithm 和 value 两个成员。
    friend bool operator==(const Checksum &, const Checksum &) = default;
};

// 校验一个 Checksum 对象是否合法。
// 合法返回 Status::Ok()；
// 非法则返回 kInvalidArgument 以及对应的错误信息。
Status ValidateChecksum(const Checksum &checksum);

} // namespace storage
