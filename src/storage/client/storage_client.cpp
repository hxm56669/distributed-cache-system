#include "storage/storage/client/storage_client.h"

#include "storage/common/unique_fd.h"

#include <blake3.h>
#include <fcntl.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <cerrno>
#include <cstring>

namespace storage {
namespace {
constexpr std::size_t kBufferSize = 1024 * 1024;
std::atomic<std::uint64_t> next_temp_id{0};

StatusOr<std::pair<std::uint64_t, Checksum>> HashFile(const std::filesystem::path &path) {
    UniqueFd fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC));
    if (!fd.valid())
        return Status{StatusCode::kIoError,
                      "open file failed: " + std::string(std::strerror(errno))};
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    std::array<std::byte, kBufferSize> buffer{};
    std::uint64_t size = 0;
    while (true) {
        ssize_t count;
        do {
            count = ::read(fd.get(), buffer.data(), buffer.size());
        } while (count < 0 && errno == EINTR);
        if (count < 0)
            return Status{StatusCode::kIoError,
                          "read file failed: " + std::string(std::strerror(errno))};
        if (count == 0)
            break;
        blake3_hasher_update(&hasher, buffer.data(), static_cast<std::size_t>(count));
        size += static_cast<std::uint64_t>(count);
    }
    std::array<std::uint8_t, BLAKE3_OUT_LEN> digest{};
    blake3_hasher_finalize(&hasher, digest.data(), digest.size());
    static constexpr char kHex[] = "0123456789abcdef";
    std::string hex(digest.size() * 2, '0');
    for (std::size_t index = 0; index < digest.size(); ++index) {
        hex[index * 2] = kHex[digest[index] >> 4U];
        hex[index * 2 + 1] = kHex[digest[index] & 0x0fU];
    }
    return std::pair{size, Checksum{ChecksumAlgorithm::kBlake3, std::move(hex)}};
}
} // namespace

StatusOr<ChunkId> StorageClient::ToChunkId(const ObjectId &object_id) {
    if (!object_id.valid())
        return Status{StatusCode::kInvalidArgument, "object_id cannot be empty"};
    return ChunkId{object_id.value};
}

Status StorageClient::Put(const ObjectId &object_id, const std::filesystem::path &source) {
    auto chunk_id = ToChunkId(object_id);
    if (!chunk_id.ok())
        return chunk_id.status();
    auto hash = HashFile(source);
    if (!hash.ok())
        return hash.status();
    return data_plane_.PutChunk(
        worker_, PutChunkRequest{chunk_id.value(), hash.value().first, hash.value().second},
        source);
}

Status StorageClient::Get(const ObjectId &object_id, const std::filesystem::path &destination) {
    auto chunk_id = ToChunkId(object_id);
    if (!chunk_id.ok())
        return chunk_id.status();
    if (std::filesystem::exists(destination))
        return {StatusCode::kAlreadyExists, "destination already exists: " + destination.string()};
    auto head = data_plane_.HeadChunk(worker_, HeadChunkRequest{chunk_id.value()});
    if (!head.ok())
        return head.status();
    const auto temp = destination.parent_path() /
                      (destination.filename().string() + ".tmp." + std::to_string(::getpid()) +
                       "." + std::to_string(next_temp_id.fetch_add(1, std::memory_order_relaxed)));
    if (std::filesystem::exists(temp))
        return {StatusCode::kAlreadyExists,
                "temporary destination already exists: " + temp.string()};
    auto status = data_plane_.GetChunk(
        worker_, GetChunkRequest{chunk_id.value(), ByteRange{0, head.value().size}}, temp);
    if (!status.ok()) {
        (void)::unlink(temp.c_str());
        return status;
    }
    auto hash = HashFile(temp);
    if (!hash.ok() || hash.value().first != head.value().size ||
        hash.value().second != head.value().checksum) {
        (void)::unlink(temp.c_str());
        return hash.ok() ? Status{StatusCode::kCorruption, "download checksum mismatch"}
                         : hash.status();
    }
    if (::link(temp.c_str(), destination.c_str()) != 0) {
        const int link_error = errno;
        (void)::unlink(temp.c_str());
        return {link_error == EEXIST ? StatusCode::kAlreadyExists : StatusCode::kIoError,
                "commit destination failed: " + std::string(std::strerror(link_error))};
    }
    (void)::unlink(temp.c_str());
    return Status::Ok();
}

StatusOr<HeadChunkResponse> StorageClient::Head(const ObjectId &object_id) {
    auto chunk_id = ToChunkId(object_id);
    if (!chunk_id.ok())
        return chunk_id.status();
    return data_plane_.HeadChunk(worker_, HeadChunkRequest{chunk_id.value()});
}

Status StorageClient::Delete(const ObjectId &object_id) {
    auto chunk_id = ToChunkId(object_id);
    if (!chunk_id.ok())
        return chunk_id.status();
    return data_plane_.DeleteChunk(worker_, DeleteChunkRequest{chunk_id.value()});
}
} // namespace storage
