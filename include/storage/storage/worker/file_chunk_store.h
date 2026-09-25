#pragma once

#include "storage/storage/worker/chunk_store.h"

#include <filesystem>

namespace storage {

class FileChunkStore final : public ChunkStore {
  public:
    explicit FileChunkStore(std::filesystem::path root);
    Status Put(const PutChunkRequest &request, ChunkInputStream &input) override;
    Status Get(const GetChunkRequest &request, ChunkOutputStream &output) const override;
    StatusOr<HeadChunkResponse> Head(const ChunkId &chunk_id) const override;
    Status Delete(const ChunkId &chunk_id) override;
    StatusOr<std::uint64_t> Size(const ChunkId &chunk_id) const override;
    bool Exists(const ChunkId &chunk_id) const override;

  private:
    Status ValidateId(const ChunkId &chunk_id) const;
    std::filesystem::path PathFor(const ChunkId &chunk_id) const;
    std::filesystem::path root_;
};

} // namespace storage
