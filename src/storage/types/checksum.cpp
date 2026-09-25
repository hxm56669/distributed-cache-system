#include "storage/storage/types/checksum.h"

#include <algorithm>
// 提供 std::ranges::all_of，用于检查所有字符是否都满足条件
#include <cctype>
// 提供 std::isdigit，用于判断字符是不是数字


namespace storage {

Status ValidateChecksum(const Checksum &checksum) {
    // 首先检查使用的哈希算法是不是项目当前支持的 BLAKE3。
    if (checksum.algorithm != ChecksumAlgorithm::kBlake3) {
        return {StatusCode::kInvalidArgument, "unsupported checksum algorithm"};
    }
    // BLAKE3 默认输出 256 bit，也就是 32 Byte。
    // 每个 Byte 用两个十六进制字符表示：
    //
    // 32 × 2 = 64
    //
    // 因此完整 BLAKE3 十六进制字符串长度必须是 64。
    constexpr std::size_t kBlake3HexLength = 64;

    // 检查 checksum 字符串长度是否正好为 64。
    if (checksum.value.size() != kBlake3HexLength) {
        return {StatusCode::kInvalidArgument,
                "BLAKE3 checksum must be 64 lowercase hex characters"};
    }
    // 检查 checksum.value 中“所有字符”是不是合法的小写十六进制字符。
    // 合法字符只能是：
    //
    // 0~9
    // a~f

    //std::ranges::all_of(checksum.value, ...)
    //遍历 checksum.value 里的每一个字符，传给 character , 检查它们是否全部满足后面的条件。

    // 参数写成 unsigned char，而不是 char，
    // 是为了安全地调用 std::isdigit。
    const bool valid = std::ranges::all_of(checksum.value, [](unsigned char character) {
        return std::isdigit(character) != 0 || (character >= 'a' && character <= 'f');
    });
    if (!valid) {
        return {StatusCode::kInvalidArgument, "BLAKE3 checksum contains invalid characters"};
    }

    // 能走到这里说明：
    //
    // 1. algorithm == kBlake3
    // 2. value.length() == 64
    // 3. value 中所有字符都属于 0~9 / a~f
    //
    // 因此 checksum 格式合法。
    return Status::Ok();
}

} // namespace storage
