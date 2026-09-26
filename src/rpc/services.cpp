#include "storage/rpc/services.h"

#include <algorithm>
#include <cstring>

namespace storage {
namespace {

// 要填写的 gRPC Health 响应对象
// 当前健康的是哪个服务。
void SetHealthy(rpc::HealthResponse *response, const char *service) {
    // mutable_status()：
    // 获取 HealthResponse 内部嵌套的 Status protobuf 对象，
    // 并且允许修改它。
    response->mutable_status()->set_code(rpc::STATUS_CODE_OK);
    response->mutable_status()->set_message("ok");
    // 设置 HealthResponse 中的 service 字段。
    //
    // Control Plane：
    // service = "control-plane"
    //
    // Worker：
    // service = "worker"
    response->set_service(service);
}

rpc::StatusCode ToRpcStatusCode(StatusCode code) {
    switch (code) {
    case StatusCode::kOk:
        return rpc::STATUS_CODE_OK;
    case StatusCode::kInvalidArgument:
        return rpc::STATUS_CODE_INVALID_ARGUMENT;
    case StatusCode::kNotFound:
        return rpc::STATUS_CODE_NOT_FOUND;
    case StatusCode::kAlreadyExists:
        return rpc::STATUS_CODE_ALREADY_EXISTS;
    case StatusCode::kUnavailable:
        return rpc::STATUS_CODE_UNAVAILABLE;
    case StatusCode::kTimeout:
        return rpc::STATUS_CODE_TIMEOUT;
    case StatusCode::kIoError:
        return rpc::STATUS_CODE_IO_ERROR;
    case StatusCode::kCorruption:
        return rpc::STATUS_CODE_CORRUPTION;
    case StatusCode::kResourceExhausted:
        return rpc::STATUS_CODE_RESOURCE_EXHAUSTED;
    case StatusCode::kInternal:
        return rpc::STATUS_CODE_INTERNAL;
    }
    return rpc::STATUS_CODE_INTERNAL;
}

void SetStatus(const Status &status, rpc::Status *response) {
    response->set_code(ToRpcStatusCode(status.code()));
    response->set_message(status.message());
}

grpc::Status ToGrpcStatus(const Status &status) {
    grpc::StatusCode code = grpc::StatusCode::INTERNAL;
    switch (status.code()) {
    case StatusCode::kOk:
        return grpc::Status::OK;
    case StatusCode::kInvalidArgument:
        code = grpc::StatusCode::INVALID_ARGUMENT;
        break;
    case StatusCode::kNotFound:
        code = grpc::StatusCode::NOT_FOUND;
        break;
    case StatusCode::kAlreadyExists:
        code = grpc::StatusCode::ALREADY_EXISTS;
        break;
    case StatusCode::kUnavailable:
        code = grpc::StatusCode::UNAVAILABLE;
        break;
    case StatusCode::kTimeout:
        code = grpc::StatusCode::DEADLINE_EXCEEDED;
        break;
    case StatusCode::kResourceExhausted:
        code = grpc::StatusCode::RESOURCE_EXHAUSTED;
        break;
    case StatusCode::kCorruption:
        code = grpc::StatusCode::DATA_LOSS;
        break;
    case StatusCode::kIoError:
        code = grpc::StatusCode::UNKNOWN;
        break;
    case StatusCode::kInternal:
        code = grpc::StatusCode::INTERNAL;
        break;
    }
    return {code, status.message()};
}

class RpcInputStream final : public ChunkInputStream {
  public:
    explicit RpcInputStream(grpc::ServerReader<rpc::PutChunkFrame> &reader) : reader_(reader) {}
    StatusOr<std::size_t> Read(std::span<std::byte> buffer) override {
        if (buffer.empty()) {
            return Status{StatusCode::kInvalidArgument, "input buffer cannot be empty"};
        }
        if (offset_ == data_.size()) {
            rpc::PutChunkFrame frame;
            if (!reader_.Read(&frame)) {
                return std::size_t{0};
            }
            if (!frame.has_data()) {
                return Status{StatusCode::kInvalidArgument,
                              "metadata must be the first frame only"};
            }
            data_ = std::move(*frame.mutable_data());
            offset_ = 0;
            if (data_.empty()) {
                return Status{StatusCode::kInvalidArgument, "empty data frame"};
            }
        }
        const auto count = std::min(buffer.size(), data_.size() - offset_);
        std::memcpy(buffer.data(), data_.data() + offset_, count);
        offset_ += count;
        return count;
    }

  private:
    grpc::ServerReader<rpc::PutChunkFrame> &reader_;
    std::string data_;
    std::size_t offset_{0};
};

class RpcOutputStream final : public ChunkOutputStream {
  public:
    explicit RpcOutputStream(grpc::ServerWriter<rpc::ChunkData> &writer) : writer_(writer) {}
    Status Write(std::span<const std::byte> data) override {
        rpc::ChunkData frame;
        frame.set_data(data.data(), data.size());
        return writer_.Write(frame) ? Status::Ok()
                                    : Status{StatusCode::kUnavailable, "client closed get stream"};
    }

  private:
    grpc::ServerWriter<rpc::ChunkData> &writer_;
};

} // namespace

grpc::Status ControlPlaneService::Health(grpc::ServerContext *, const rpc::HealthRequest *,
                                         rpc::HealthResponse *response) {
    SetHealthy(response, "control-plane");
    return grpc::Status::OK;
}

grpc::Status WorkerService::Health(grpc::ServerContext *, const rpc::HealthRequest *,
                                   rpc::HealthResponse *response) {
    SetHealthy(response, "worker");
    return grpc::Status::OK;
}

grpc::Status WorkerService::PutChunk(grpc::ServerContext *,
                                     grpc::ServerReader<rpc::PutChunkFrame> *reader,
                                     rpc::OperationResponse *response) {
    if (data_plane_ == nullptr) {
        return {grpc::StatusCode::FAILED_PRECONDITION, "data plane is not configured"};
    }
    rpc::PutChunkFrame first;
    if (!reader->Read(&first) || !first.has_metadata()) {
        return {grpc::StatusCode::INVALID_ARGUMENT, "first put frame must contain metadata"};
    }
    const auto &metadata = first.metadata();
    PutChunkRequest request{ChunkId{metadata.chunk_id()}, metadata.expected_size(),
                            Checksum{ChecksumAlgorithm::kBlake3, metadata.expected_checksum()}};
    RpcInputStream input(*reader);
    const auto status = data_plane_->PutChunk(request, input);
    SetStatus(status, response->mutable_status());
    return grpc::Status::OK;
}

grpc::Status WorkerService::GetChunk(grpc::ServerContext *, const rpc::GetChunkRequest *request,
                                     grpc::ServerWriter<rpc::ChunkData> *writer) {
    if (data_plane_ == nullptr) {
        return {grpc::StatusCode::FAILED_PRECONDITION, "data plane is not configured"};
    }
    RpcOutputStream output(*writer);
    return ToGrpcStatus(
        data_plane_->GetChunk(GetChunkRequest{ChunkId{request->chunk_id()},
                                              ByteRange{request->offset(), request->size()}},
                              output));
}

grpc::Status WorkerService::HeadChunk(grpc::ServerContext *, const rpc::HeadChunkRequest *request,
                                      rpc::HeadChunkResponse *response) {
    if (data_plane_ == nullptr) {
        return {grpc::StatusCode::FAILED_PRECONDITION, "data plane is not configured"};
    }
    auto result = data_plane_->HeadChunk(HeadChunkRequest{ChunkId{request->chunk_id()}});
    if (!result.ok()) {
        SetStatus(result.status(), response->mutable_status());
        return grpc::Status::OK;
    }
    SetStatus(Status::Ok(), response->mutable_status());
    response->set_chunk_id(result.value().chunk_id.value);
    response->set_size(result.value().size);
    response->set_checksum(result.value().checksum.value);
    return grpc::Status::OK;
}

grpc::Status WorkerService::DeleteChunk(grpc::ServerContext *,
                                        const rpc::DeleteChunkRequest *request,
                                        rpc::OperationResponse *response) {
    if (data_plane_ == nullptr) {
        return {grpc::StatusCode::FAILED_PRECONDITION, "data plane is not configured"};
    }
    SetStatus(data_plane_->DeleteChunk(DeleteChunkRequest{ChunkId{request->chunk_id()}}),
              response->mutable_status());
    return grpc::Status::OK;
}

} // namespace storage
