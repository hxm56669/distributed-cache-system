#include "storage/storage/worker/file_chunk_store.h"

#include "storage/common/unique_fd.h"

#include <blake3.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <array>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <limits>
#include <string>
#include <system_error>
#include <vector>

namespace storage {
namespace {

constexpr std::array<char, 8> kMagic{'S', 'T', 'C', 'H', 'V', '1', '\0', '\0'};
constexpr std::size_t kChecksumLength = 64;
constexpr std::size_t kHeaderSize = kMagic.size() + sizeof(std::uint64_t) + kChecksumLength;
constexpr std::size_t kBufferSize = 3 * 1024 * 1024;

Status IoError(const std::string &operation, const std::filesystem::path &path) {
    return {StatusCode::kIoError, operation + " " + path.string() + ": " + std::strerror(errno)};
}

Status WriteAll(int fd, std::span<const std::byte> data) {
    std::size_t written = 0;
    while (written < data.size()) {
        const auto result = ::write(fd, data.data() + written, data.size() - written);
        if (result < 0 && errno == EINTR) {
            continue;
        }
        if (result <= 0) {
            return {StatusCode::kIoError, "write failed: " + std::string(std::strerror(errno))};
        }
        written += static_cast<std::size_t>(result);
    }
    return Status::Ok();
}

StatusOr<std::size_t> PreadSome(int fd, std::span<std::byte> data, std::uint64_t offset) {
    if (offset > static_cast<std::uint64_t>(std::numeric_limits<off_t>::max())) {
        return Status{StatusCode::kInvalidArgument, "file offset exceeds platform limit"};
    }
    while (true) {
        const auto result = ::pread(fd, data.data(), data.size(), static_cast<off_t>(offset));
        if (result < 0 && errno == EINTR) {
            continue;
        }
        if (result < 0) {
            return Status{StatusCode::kIoError,
                          "pread failed: " + std::string(std::strerror(errno))};
        }
        return static_cast<std::size_t>(result);
    }
}

std::array<std::byte, kHeaderSize> MakeHeader(std::uint64_t size, const Checksum &checksum) {
    std::array<std::byte, kHeaderSize> header{};
    std::memcpy(header.data(), kMagic.data(), kMagic.size());
    for (std::size_t index = 0; index < sizeof(size); ++index) {
        header[kMagic.size() + index] = static_cast<std::byte>((size >> (index * 8U)) & 0xffU);
    }
    std::memcpy(header.data() + kMagic.size() + sizeof(size), checksum.value.data(),
                kChecksumLength);
    return header;
}

StatusOr<HeadChunkResponse> ReadHeader(int fd, const ChunkId &chunk_id) {
    std::array<std::byte, kHeaderSize> header{};
    std::size_t read = 0;
    while (read < header.size()) {
        auto result = PreadSome(fd, std::span<std::byte>(header).subspan(read), read);
        if (!result.ok()) {
            return result.status();
        }
        if (result.value() == 0) {
            return Status{StatusCode::kCorruption, "truncated chunk header"};
        }
        read += result.value();
    }
    if (std::memcmp(header.data(), kMagic.data(), kMagic.size()) != 0) {
        return Status{StatusCode::kCorruption, "invalid chunk file magic"};
    }
    std::uint64_t size = 0;
    for (std::size_t index = 0; index < sizeof(size); ++index) {
        size |= static_cast<std::uint64_t>(header[kMagic.size() + index]) << (index * 8U);
    }
    Checksum checksum{
        ChecksumAlgorithm::kBlake3,
        std::string(reinterpret_cast<const char *>(header.data() + kMagic.size() + sizeof(size)),
                    kChecksumLength)};
    auto valid = ValidateChecksum(checksum);
    if (!valid.ok()) {
        return Status{StatusCode::kCorruption, "invalid persisted checksum"};
    }
    struct stat file_stat {};
    if (::fstat(fd, &file_stat) != 0) {
        return Status{StatusCode::kIoError, "fstat failed: " + std::string(std::strerror(errno))};
    }
    if (file_stat.st_size < 0 || static_cast<std::uint64_t>(file_stat.st_size) !=
                                     size + static_cast<std::uint64_t>(kHeaderSize)) {
        return Status{StatusCode::kCorruption, "chunk file size does not match header"};
    }
    return HeadChunkResponse{chunk_id, size, std::move(checksum)};
}

std::string FinalizeHex(blake3_hasher &hasher) {
    std::array<std::uint8_t, BLAKE3_OUT_LEN> digest{};
    blake3_hasher_finalize(&hasher, digest.data(), digest.size());
    static constexpr char kHex[] = "0123456789abcdef";
    std::string result(digest.size() * 2, '0');
    for (std::size_t index = 0; index < digest.size(); ++index) {
        result[index * 2] = kHex[digest[index] >> 4U];
        result[index * 2 + 1] = kHex[digest[index] & 0x0fU];
    }
    return result;
}

} // namespace

FileChunkStore::FileChunkStore(std::filesystem::path root) : root_(std::move(root)) {}

Status FileChunkStore::ValidateId(const ChunkId &chunk_id) const {
    if (!chunk_id.valid()) {
        return {StatusCode::kInvalidArgument, "chunk_id cannot be empty"};
    }
    if (chunk_id.value.size() > 1024) {
        return {StatusCode::kInvalidArgument, "chunk_id is too long"};
    }
    return Status::Ok();
}

std::filesystem::path FileChunkStore::PathFor(const ChunkId &chunk_id) const {
    static constexpr char kHex[] = "0123456789abcdef";
    std::string encoded;
    encoded.reserve(chunk_id.value.size() * 2);
    for (const unsigned char character : chunk_id.value) {
        encoded.push_back(kHex[character >> 4U]);
        encoded.push_back(kHex[character & 0x0fU]);
    }
    return root_ / (encoded + ".chunk");
}

Status FileChunkStore::Put(const PutChunkRequest &request, ChunkInputStream &input) {
    if (auto status = ValidateId(request.chunk_id); !status.ok()) {
        return status;
    }
    if (auto status = ValidateChecksum(request.expected_checksum); !status.ok()) {
        return status;
    }
    std::error_code error;
    std::filesystem::create_directories(root_, error);
    if (error) {
        return {StatusCode::kIoError, "create storage root failed: " + error.message()};
    }
    const auto final_path = PathFor(request.chunk_id);
    const auto temp_path =
        root_ / (final_path.filename().string() + ".tmp." + std::to_string(::getpid()) + "." +
                 std::to_string(reinterpret_cast<std::uintptr_t>(&input)));
    UniqueFd fd(::open(temp_path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600));
    if (!fd.valid()) {
        return errno == EEXIST ? Status{StatusCode::kUnavailable, "temporary file collision"}
                               : IoError("create", temp_path);
    }
    const auto cleanup = [&]() { (void)::unlink(temp_path.c_str()); };
    const auto header = MakeHeader(request.expected_size, request.expected_checksum);
    if (auto status = WriteAll(fd.get(), header); !status.ok()) {
        cleanup();
        return status;
    }

    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    std::vector<std::byte> buffer(kBufferSize);
    std::uint64_t received = 0;
    while (true) {
        auto count = input.Read(buffer);
        if (!count.ok()) {
            cleanup();
            return count.status();
        }
        if (count.value() == 0) {
            break;
        }
        if (count.value() > buffer.size() || received > request.expected_size ||
            count.value() > request.expected_size - received) {
            cleanup();
            return {StatusCode::kCorruption, "received more bytes than expected"};
        }
        const auto bytes = std::span<const std::byte>(buffer).first(count.value());
        blake3_hasher_update(&hasher, bytes.data(), bytes.size());
        if (auto status = WriteAll(fd.get(), bytes); !status.ok()) {
            cleanup();
            return status;
        }
        received += count.value();
    }
    if (received != request.expected_size) {
        cleanup();
        return {StatusCode::kCorruption, "received byte count does not match expected_size"};
    }
    if (FinalizeHex(hasher) != request.expected_checksum.value) {
        cleanup();
        return {StatusCode::kCorruption, "BLAKE3 checksum mismatch"};
    }
    if (::fdatasync(fd.get()) != 0) {
        const auto status = IoError("fdatasync", temp_path);
        cleanup();
        return status;
    }
    fd.reset();
    if (::link(temp_path.c_str(), final_path.c_str()) != 0) {
        const int link_error = errno;
        cleanup();
        if (link_error == EEXIST) {
            return {StatusCode::kAlreadyExists, "chunk already exists: " + request.chunk_id.value};
        }
        errno = link_error;
        return IoError("commit", final_path);
    }
    cleanup();
    UniqueFd directory_fd(::open(root_.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC));
    if (!directory_fd.valid() || ::fsync(directory_fd.get()) != 0) {
        return IoError("fsync directory", root_);
    }
    return Status::Ok();
}

StatusOr<HeadChunkResponse> FileChunkStore::Head(const ChunkId &chunk_id) const {
    if (auto status = ValidateId(chunk_id); !status.ok()) {
        return status;
    }
    const auto path = PathFor(chunk_id);
    UniqueFd fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW));
    if (!fd.valid()) {
        return errno == ENOENT ? Status{StatusCode::kNotFound, "chunk not found: " + chunk_id.value}
                               : IoError("open", path);
    }
    return ReadHeader(fd.get(), chunk_id);
}

Status FileChunkStore::Get(const GetChunkRequest &request, ChunkOutputStream &output) const {
    auto head = Head(request.chunk_id);
    if (!head.ok()) {
        return head.status();
    }
    if (auto status = ValidateByteRange(request.range, head.value().size); !status.ok()) {
        return status;
    }
    const auto path = PathFor(request.chunk_id);
    UniqueFd fd(::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NOFOLLOW));
    if (!fd.valid()) {
        return errno == ENOENT
                   ? Status{StatusCode::kNotFound, "chunk not found: " + request.chunk_id.value}
                   : IoError("open", path);
    }
    std::vector<std::byte> buffer(kBufferSize);
    std::uint64_t remaining = request.range.size;
    std::uint64_t offset = request.range.offset + kHeaderSize;
    while (remaining > 0) {
        const auto wanted =
            static_cast<std::size_t>(std::min<std::uint64_t>(remaining, buffer.size()));
        auto count = PreadSome(fd.get(), std::span<std::byte>(buffer).first(wanted), offset);
        if (!count.ok()) {
            return count.status();
        }
        if (count.value() == 0) {
            return {StatusCode::kCorruption, "unexpected EOF while reading chunk"};
        }
        if (auto status = output.Write(std::span<const std::byte>(buffer).first(count.value()));
            !status.ok()) {
            return status;
        }
        remaining -= count.value();
        offset += count.value();
    }
    return Status::Ok();
}

Status FileChunkStore::Delete(const ChunkId &chunk_id) {
    if (auto status = ValidateId(chunk_id); !status.ok()) {
        return status;
    }
    const auto path = PathFor(chunk_id);
    if (::unlink(path.c_str()) != 0) {
        return errno == ENOENT ? Status{StatusCode::kNotFound, "chunk not found: " + chunk_id.value}
                               : IoError("delete", path);
    }
    return Status::Ok();
}

StatusOr<std::uint64_t> FileChunkStore::Size(const ChunkId &chunk_id) const {
    auto head = Head(chunk_id);
    return head.ok() ? StatusOr<std::uint64_t>(head.value().size)
                     : StatusOr<std::uint64_t>(head.status());
}

bool FileChunkStore::Exists(const ChunkId &chunk_id) const { return Head(chunk_id).ok(); }

} // namespace storage
