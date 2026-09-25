#include "storage/storage/worker/data_plane_service.h"

namespace storage {

Status DataPlaneService::PutChunk(const PutChunkRequest &request, ChunkInputStream &input) {
    if (!request.chunk_id.valid()) {
        return {StatusCode::kInvalidArgument, "chunk_id cannot be empty"};
    }
    if (auto status = ValidateChecksum(request.expected_checksum); !status.ok()) {
        return status;
    }
    return store_.Put(request, input);
}

Status DataPlaneService::GetChunk(const GetChunkRequest &request, ChunkOutputStream &output) const {
    if (!request.chunk_id.valid()) {
        return {StatusCode::kInvalidArgument, "chunk_id cannot be empty"};
    }
    return store_.Get(request, output);
}

StatusOr<HeadChunkResponse> DataPlaneService::HeadChunk(const HeadChunkRequest &request) const {
    if (!request.chunk_id.valid()) {
        return Status{StatusCode::kInvalidArgument, "chunk_id cannot be empty"};
    }
    return store_.Head(request.chunk_id);
}

Status DataPlaneService::DeleteChunk(const DeleteChunkRequest &request) {
    if (!request.chunk_id.valid()) {
        return {StatusCode::kInvalidArgument, "chunk_id cannot be empty"};
    }
    return store_.Delete(request.chunk_id);
}

} // namespace storage
