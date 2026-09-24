#pragma once

#include "storage/common/status.h"

#include <cstdint>

namespace storage {

struct ByteRange {
    std::uint64_t offset;
    std::uint64_t size;
};

Status ValidateByteRange(const ByteRange &range, std::uint64_t total_size);

} // namespace storage
