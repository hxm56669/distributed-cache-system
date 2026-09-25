#pragma once

#include "storage/storage/types/byte_range.h"
#include "storage/storage/types/checksum.h"
#include "storage/storage/types/chunk_id.h"

#include <cstdint>
#include <string>

namespace storage {

struct WorkerEndpoint {
    std::string address;
};
struct PutChunkRequest {
    ChunkId chunk_id;
    std::uint64_t expected_size;
    Checksum expected_checksum;
};
struct GetChunkRequest {
    ChunkId chunk_id;
    ByteRange range;
};
struct DeleteChunkRequest {
    ChunkId chunk_id;
};
struct HeadChunkRequest {
    ChunkId chunk_id;
};
struct HeadChunkResponse {
    ChunkId chunk_id;
    std::uint64_t size;
    Checksum checksum;
};

} // namespace storage
