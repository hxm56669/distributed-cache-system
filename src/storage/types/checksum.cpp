#include "storage/storage/types/checksum.h"

#include <algorithm>
#include <cctype>

namespace storage {

Status ValidateChecksum(const Checksum &checksum) {
    if (checksum.algorithm != ChecksumAlgorithm::kBlake3) {
        return {StatusCode::kInvalidArgument, "unsupported checksum algorithm"};
    }
    constexpr std::size_t kBlake3HexLength = 64;
    if (checksum.value.size() != kBlake3HexLength) {
        return {StatusCode::kInvalidArgument,
                "BLAKE3 checksum must be 64 lowercase hex characters"};
    }
    const bool valid = std::ranges::all_of(checksum.value, [](unsigned char character) {
        return std::isdigit(character) != 0 || (character >= 'a' && character <= 'f');
    });
    if (!valid) {
        return {StatusCode::kInvalidArgument, "BLAKE3 checksum contains invalid characters"};
    }
    return Status::Ok();
}

} // namespace storage
