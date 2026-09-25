#pragma once

#include "storage/common/status_or.h"
#include "storage/storage/worker/data_plane_types.h"

#include <chrono>
#include <filesystem>

namespace storage {

class DataPlaneClient {
  public:
    virtual ~DataPlaneClient() = default;
    virtual Status PutChunk(const WorkerEndpoint &endpoint, const PutChunkRequest &request,
                            const std::filesystem::path &source) = 0;
    virtual Status GetChunk(const WorkerEndpoint &endpoint, const GetChunkRequest &request,
                            const std::filesystem::path &destination) = 0;
    virtual StatusOr<HeadChunkResponse> HeadChunk(const WorkerEndpoint &endpoint,
                                                  const HeadChunkRequest &request) = 0;
    virtual Status DeleteChunk(const WorkerEndpoint &endpoint,
                               const DeleteChunkRequest &request) = 0;
};

class GrpcDataPlaneClient final : public DataPlaneClient {
  public:
    explicit GrpcDataPlaneClient(std::chrono::seconds deadline = std::chrono::seconds(300))
        : deadline_(deadline) {}
    Status PutChunk(const WorkerEndpoint &, const PutChunkRequest &,
                    const std::filesystem::path &) override;
    Status GetChunk(const WorkerEndpoint &, const GetChunkRequest &,
                    const std::filesystem::path &) override;
    StatusOr<HeadChunkResponse> HeadChunk(const WorkerEndpoint &,
                                          const HeadChunkRequest &) override;
    Status DeleteChunk(const WorkerEndpoint &, const DeleteChunkRequest &) override;

  private:
    std::chrono::seconds deadline_;
};

} // namespace storage
