#pragma once

#include "storage/storage/worker/data_plane_types.h"
#include "storage/storage/worker/streams.h"

namespace storage {

class ChunkStore {
  public:
    virtual ~ChunkStore() = default;
    virtual Status Put(const PutChunkRequest &request, ChunkInputStream &input) = 0;
    virtual Status Get(const GetChunkRequest &request, ChunkOutputStream &output) const = 0;
    virtual StatusOr<HeadChunkResponse> Head(const ChunkId &chunk_id) const = 0;
    virtual Status Delete(const ChunkId &chunk_id) = 0;
    virtual StatusOr<std::uint64_t> Size(const ChunkId &chunk_id) const = 0;
    virtual bool Exists(const ChunkId &chunk_id) const = 0;
};

} // namespace storage
