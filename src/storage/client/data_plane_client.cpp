#include "storage/storage/client/data_plane_client.h"

#include "storage/common/unique_fd.h"

#include "worker.grpc.pb.h"

#include <grpcpp/create_channel.h>
#include <grpcpp/resource_quota.h>
#include <grpcpp/security/credentials.h>

#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstring>
#include <memory>
#include <string>

namespace storage {
namespace {

constexpr std::size_t kBufferSize = 3 * 1024 * 1024;

StatusCode FromRpcCode(rpc::StatusCode code) {
    switch (code) {
    case rpc::STATUS_CODE_OK:
        return StatusCode::kOk;
    case rpc::STATUS_CODE_INVALID_ARGUMENT:
        return StatusCode::kInvalidArgument;
    case rpc::STATUS_CODE_NOT_FOUND:
        return StatusCode::kNotFound;
    case rpc::STATUS_CODE_ALREADY_EXISTS:
        return StatusCode::kAlreadyExists;
    case rpc::STATUS_CODE_UNAVAILABLE:
        return StatusCode::kUnavailable;
    case rpc::STATUS_CODE_TIMEOUT:
        return StatusCode::kTimeout;
    case rpc::STATUS_CODE_IO_ERROR:
        return StatusCode::kIoError;
    case rpc::STATUS_CODE_CORRUPTION:
        return StatusCode::kCorruption;
    case rpc::STATUS_CODE_RESOURCE_EXHAUSTED:
        return StatusCode::kResourceExhausted;
    case rpc::STATUS_CODE_INTERNAL:
    default:
        return StatusCode::kInternal;
    }
}

Status FromRpcStatus(const rpc::Status &status) {
    return status.code() == rpc::STATUS_CODE_OK
               ? Status::Ok()
               : Status{FromRpcCode(status.code()), status.message()};
}

Status FromGrpcStatus(const grpc::Status &status) {
    if (status.ok())
        return Status::Ok();
    StatusCode code = StatusCode::kUnavailable;
    switch (status.error_code()) {
    case grpc::StatusCode::INVALID_ARGUMENT:
        code = StatusCode::kInvalidArgument;
        break;
    case grpc::StatusCode::NOT_FOUND:
        code = StatusCode::kNotFound;
        break;
    case grpc::StatusCode::ALREADY_EXISTS:
        code = StatusCode::kAlreadyExists;
        break;
    case grpc::StatusCode::DEADLINE_EXCEEDED:
        code = StatusCode::kTimeout;
        break;
    case grpc::StatusCode::RESOURCE_EXHAUSTED:
        code = StatusCode::kResourceExhausted;
        break;
    case grpc::StatusCode::INTERNAL:
        code = StatusCode::kInternal;
        break;
    default:
        break;
    }
    return {code, status.error_message()};
}

std::unique_ptr<rpc::WorkerRpc::Stub> MakeStub(const WorkerEndpoint &endpoint) {
    grpc::ResourceQuota quota;
    quota.Resize(256 * 1024 * 1024);
    grpc::ChannelArguments arguments;
    arguments.SetResourceQuota(quota);
    return rpc::WorkerRpc::NewStub(grpc::CreateCustomChannel(
        endpoint.address, grpc::InsecureChannelCredentials(), arguments));
}

Status WriteAll(int fd, const char *data, std::size_t size) {
    std::size_t written = 0;
    while (written < size) {
        const auto result = ::write(fd, data + written, size - written);
        if (result < 0 && errno == EINTR)
            continue;
        if (result <= 0) {
            return {StatusCode::kIoError, "write failed: " + std::string(std::strerror(errno))};
        }
        written += static_cast<std::size_t>(result);
    }
    return Status::Ok();
}

} // namespace

Status GrpcDataPlaneClient::PutChunk(const WorkerEndpoint &endpoint, const PutChunkRequest &request,
                                     const std::filesystem::path &source) {
    UniqueFd fd(::open(source.c_str(), O_RDONLY | O_CLOEXEC));
    if (!fd.valid()) {
        return {StatusCode::kIoError, "open source failed: " + std::string(std::strerror(errno))};
    }
    auto stub = MakeStub(endpoint);
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + deadline_);
    rpc::OperationResponse response;
    auto writer = stub->PutChunk(&context, &response);
    rpc::PutChunkFrame metadata;
    metadata.mutable_metadata()->set_chunk_id(request.chunk_id.value);
    metadata.mutable_metadata()->set_expected_size(request.expected_size);
    metadata.mutable_metadata()->set_expected_checksum(request.expected_checksum.value);
    if (!writer->Write(metadata)) {
        writer->WritesDone();
        return FromGrpcStatus(writer->Finish());
    }
    std::array<char, kBufferSize> buffer{};
    while (true) {
        ssize_t count;
        do {
            count = ::read(fd.get(), buffer.data(), buffer.size());
        } while (count < 0 && errno == EINTR);
        if (count < 0) {
            context.TryCancel();
            writer->WritesDone();
            (void)writer->Finish();
            return {StatusCode::kIoError,
                    "read source failed: " + std::string(std::strerror(errno))};
        }
        if (count == 0)
            break;
        rpc::PutChunkFrame frame;
        frame.set_data(buffer.data(), static_cast<std::size_t>(count));
        if (!writer->Write(frame))
            break;
    }
    writer->WritesDone();
    const auto grpc_status = writer->Finish();
    if (!grpc_status.ok())
        return FromGrpcStatus(grpc_status);
    return FromRpcStatus(response.status());
}

Status GrpcDataPlaneClient::GetChunk(const WorkerEndpoint &endpoint, const GetChunkRequest &request,
                                     const std::filesystem::path &destination) {
    UniqueFd fd(::open(destination.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600));
    if (!fd.valid()) {
        return {errno == EEXIST ? StatusCode::kAlreadyExists : StatusCode::kIoError,
                "create destination failed: " + std::string(std::strerror(errno))};
    }
    auto stub = MakeStub(endpoint);
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + deadline_);
    rpc::GetChunkRequest rpc_request;
    rpc_request.set_chunk_id(request.chunk_id.value);
    rpc_request.set_offset(request.range.offset);
    rpc_request.set_size(request.range.size);
    auto reader = stub->GetChunk(&context, rpc_request);
    rpc::ChunkData frame;
    Status write_status = Status::Ok();
    while (reader->Read(&frame)) {
        write_status = WriteAll(fd.get(), frame.data().data(), frame.data().size());
        if (!write_status.ok()) {
            context.TryCancel();
            break;
        }
    }
    const auto grpc_status = reader->Finish();
    if (!write_status.ok())
        return write_status;
    if (!grpc_status.ok())
        return FromGrpcStatus(grpc_status);
    if (::fdatasync(fd.get()) != 0) {
        return {StatusCode::kIoError,
                "fdatasync destination failed: " + std::string(std::strerror(errno))};
    }
    return Status::Ok();
}

StatusOr<HeadChunkResponse> GrpcDataPlaneClient::HeadChunk(const WorkerEndpoint &endpoint,
                                                           const HeadChunkRequest &request) {
    auto stub = MakeStub(endpoint);
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + deadline_);
    rpc::HeadChunkRequest rpc_request;
    rpc_request.set_chunk_id(request.chunk_id.value);
    rpc::HeadChunkResponse response;
    const auto grpc_status = stub->HeadChunk(&context, rpc_request, &response);
    if (!grpc_status.ok())
        return FromGrpcStatus(grpc_status);
    const auto status = FromRpcStatus(response.status());
    if (!status.ok())
        return status;
    return HeadChunkResponse{ChunkId{response.chunk_id()}, response.size(),
                             Checksum{ChecksumAlgorithm::kBlake3, response.checksum()}};
}

Status GrpcDataPlaneClient::DeleteChunk(const WorkerEndpoint &endpoint,
                                        const DeleteChunkRequest &request) {
    auto stub = MakeStub(endpoint);
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + deadline_);
    rpc::DeleteChunkRequest rpc_request;
    rpc_request.set_chunk_id(request.chunk_id.value);
    rpc::OperationResponse response;
    const auto grpc_status = stub->DeleteChunk(&context, rpc_request, &response);
    return grpc_status.ok() ? FromRpcStatus(response.status()) : FromGrpcStatus(grpc_status);
}

} // namespace storage
