#pragma once

#include "storage/storage/worker/chunk_store.h"

namespace storage {

class DataPlaneService {
  public:
    explicit DataPlaneService(ChunkStore &store) : store_(store) {}
    Status PutChunk(const PutChunkRequest &request, ChunkInputStream &input);
    Status GetChunk(const GetChunkRequest &request, ChunkOutputStream &output) const;
    StatusOr<HeadChunkResponse> HeadChunk(const HeadChunkRequest &request) const;
    Status DeleteChunk(const DeleteChunkRequest &request);

  private:
    ChunkStore &store_;
};

} // namespace storage
