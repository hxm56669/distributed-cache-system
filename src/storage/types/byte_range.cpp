#include "storage/storage/types/byte_range.h"

namespace storage {

Status ValidateByteRange(const ByteRange &range, std::uint64_t total_size) {
    if (range.offset > total_size) {
        return {StatusCode::kInvalidArgument, "byte range offset exceeds object size"};
    }
    if (range.size > total_size - range.offset) {
        return {StatusCode::kInvalidArgument, "byte range size exceeds object boundary"};
    }
    return Status::Ok();
}

} // namespace storage
