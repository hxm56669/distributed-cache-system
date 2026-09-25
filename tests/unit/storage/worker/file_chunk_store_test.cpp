#include "storage/storage/worker/file_chunk_store.h"

#include <blake3.h>
#include <gtest/gtest.h>

#include <array>
#include <cstring>
#include <filesystem>
#include <span>
#include <vector>

namespace storage {
namespace {

class VectorInput final : public ChunkInputStream {
  public:
    explicit VectorInput(std::span<const std::byte> data) : data_(data) {}
    StatusOr<std::size_t> Read(std::span<std::byte> buffer) override {
        const auto count = std::min(buffer.size(), data_.size() - offset_);
        std::memcpy(buffer.data(), data_.data() + offset_, count);
        offset_ += count;
        return count;
    }

  private:
    std::span<const std::byte> data_;
    std::size_t offset_{0};
};

class VectorOutput final : public ChunkOutputStream {
  public:
    Status Write(std::span<const std::byte> data) override {
        value.insert(value.end(), data.begin(), data.end());
        return Status::Ok();
    }
    std::vector<std::byte> value;
};

Checksum Hash(std::span<const std::byte> data) {
    std::array<std::uint8_t, BLAKE3_OUT_LEN> digest{};
    blake3_hasher hasher;
    blake3_hasher_init(&hasher);
    blake3_hasher_update(&hasher, data.data(), data.size());
    blake3_hasher_finalize(&hasher, digest.data(), digest.size());
    static constexpr char kHex[] = "0123456789abcdef";
    std::string result(digest.size() * 2, '0');
    for (std::size_t index = 0; index < digest.size(); ++index) {
        result[index * 2] = kHex[digest[index] >> 4U];
        result[index * 2 + 1] = kHex[digest[index] & 0x0fU];
    }
    return {ChecksumAlgorithm::kBlake3, std::move(result)};
}

class FileChunkStoreTest : public ::testing::Test {
  protected:
    void SetUp() override {
        root = std::filesystem::temp_directory_path() /
               ("storage-unit-" + std::to_string(reinterpret_cast<std::uintptr_t>(this)));
        std::filesystem::remove_all(root);
    }
    void TearDown() override { std::filesystem::remove_all(root); }
    std::filesystem::path root;
};

TEST_F(FileChunkStoreTest, PutGetHeadAndRestart) {
    std::vector<std::byte> data(4096, std::byte{0x5a});
    const PutChunkRequest request{ChunkId{"alpha"}, data.size(), Hash(data)};
    VectorInput input(data);
    FileChunkStore store(root);
    ASSERT_TRUE(store.Put(request, input).ok());
    EXPECT_TRUE(store.Exists(request.chunk_id));
    ASSERT_TRUE(store.Size(request.chunk_id).ok());
    EXPECT_EQ(store.Size(request.chunk_id).value(), data.size());

    FileChunkStore restarted(root);
    auto head = restarted.Head(request.chunk_id);
    ASSERT_TRUE(head.ok());
    EXPECT_EQ(head.value().checksum, request.expected_checksum);
    VectorOutput output;
    ASSERT_TRUE(
        restarted.Get(GetChunkRequest{request.chunk_id, ByteRange{0, data.size()}}, output).ok());
    EXPECT_EQ(output.value, data);
}

TEST_F(FileChunkStoreTest, DuplicatePutDoesNotOverwrite) {
    std::vector<std::byte> first(32, std::byte{1});
    std::vector<std::byte> second(32, std::byte{2});
    FileChunkStore store(root);
    VectorInput first_input(first);
    ASSERT_TRUE(store.Put({ChunkId{"same"}, first.size(), Hash(first)}, first_input).ok());
    VectorInput second_input(second);
    EXPECT_EQ(store.Put({ChunkId{"same"}, second.size(), Hash(second)}, second_input).code(),
              StatusCode::kAlreadyExists);
    VectorOutput output;
    ASSERT_TRUE(store.Get({ChunkId{"same"}, ByteRange{0, first.size()}}, output).ok());
    EXPECT_EQ(output.value, first);
}

TEST_F(FileChunkStoreTest, MissingRangeAndDeleteSemantics) {
    FileChunkStore store(root);
    VectorOutput missing;
    EXPECT_EQ(store.Get({ChunkId{"missing"}, ByteRange{0, 0}}, missing).code(),
              StatusCode::kNotFound);
    std::vector<std::byte> data{std::byte{0}, std::byte{1}, std::byte{2}, std::byte{3}};
    VectorInput input(data);
    ASSERT_TRUE(store.Put({ChunkId{"range"}, data.size(), Hash(data)}, input).ok());
    VectorOutput range;
    ASSERT_TRUE(store.Get({ChunkId{"range"}, ByteRange{1, 2}}, range).ok());
    EXPECT_EQ(range.value, (std::vector<std::byte>{std::byte{1}, std::byte{2}}));
    VectorOutput invalid;
    EXPECT_EQ(store.Get({ChunkId{"range"}, ByteRange{3, 2}}, invalid).code(),
              StatusCode::kInvalidArgument);
    EXPECT_TRUE(store.Delete(ChunkId{"range"}).ok());
    EXPECT_FALSE(store.Exists(ChunkId{"range"}));
    EXPECT_EQ(store.Delete(ChunkId{"range"}).code(), StatusCode::kNotFound);
}

TEST_F(FileChunkStoreTest, RejectsInvalidIdAndFailedWritesLeaveNoChunk) {
    std::vector<std::byte> data(8, std::byte{3});
    FileChunkStore store(root);
    VectorInput empty_id_input(data);
    EXPECT_EQ(store.Put({ChunkId{""}, data.size(), Hash(data)}, empty_id_input).code(),
              StatusCode::kInvalidArgument);
    auto wrong = Hash(data);
    wrong.value[0] = wrong.value[0] == '0' ? '1' : '0';
    VectorInput bad_hash_input(data);
    EXPECT_EQ(store.Put({ChunkId{"bad-hash"}, data.size(), wrong}, bad_hash_input).code(),
              StatusCode::kCorruption);
    VectorInput bad_size_input(data);
    EXPECT_EQ(store.Put({ChunkId{"bad-size"}, data.size() + 1, Hash(data)}, bad_size_input).code(),
              StatusCode::kCorruption);
    EXPECT_FALSE(store.Exists(ChunkId{"bad-hash"}));
    EXPECT_FALSE(store.Exists(ChunkId{"bad-size"}));
}

TEST_F(FileChunkStoreTest, EncodesTraversalAndAbsoluteIdsInsideRoot) {
    std::vector<std::byte> data{std::byte{7}};
    FileChunkStore store(root);
    for (const std::string id : {"../escape", "/tmp/escape", "a/b"}) {
        VectorInput input(data);
        ASSERT_TRUE(store.Put({ChunkId{id}, data.size(), Hash(data)}, input).ok());
        EXPECT_TRUE(store.Exists(ChunkId{id}));
    }
    for (const auto &entry : std::filesystem::directory_iterator(root)) {
        EXPECT_EQ(entry.path().parent_path(), root);
    }
}

} // namespace
} // namespace storage
