#pragma once

#include "storage/storage/client/data_plane_client.h"
#include "storage/storage/types/object_id.h"

namespace storage {

class StorageClient {
  public:
    StorageClient(DataPlaneClient &data_plane, WorkerEndpoint worker)
        : data_plane_(data_plane), worker_(std::move(worker)) {}
    Status Put(const ObjectId &object_id, const std::filesystem::path &source);
    Status Get(const ObjectId &object_id, const std::filesystem::path &destination);
    StatusOr<HeadChunkResponse> Head(const ObjectId &object_id);
    Status Delete(const ObjectId &object_id);

  private:
    static StatusOr<ChunkId> ToChunkId(const ObjectId &object_id);
    DataPlaneClient &data_plane_;
    WorkerEndpoint worker_;
};

} // namespace storage
