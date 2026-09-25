#include "storage/rpc/services.h"
#include "storage/storage/client/data_plane_client.h"
#include "storage/storage/client/storage_client.h"
#include "storage/storage/worker/data_plane_service.h"
#include "storage/storage/worker/file_chunk_store.h"

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>

namespace storage {
namespace {

void WritePattern(const std::filesystem::path &path, std::uint64_t size) {
    std::ofstream output(path, std::ios::binary);
    std::string block(1024 * 1024, '\0');
    for (std::size_t index = 0; index < block.size(); ++index)
        block[index] = static_cast<char>(index);
    while (size > 0) {
        const auto count =
            static_cast<std::streamsize>(std::min<std::uint64_t>(size, block.size()));
        output.write(block.data(), count);
        size -= static_cast<std::uint64_t>(count);
    }
}

void ExpectFilesEqual(const std::filesystem::path &left, const std::filesystem::path &right) {
    std::ifstream a(left, std::ios::binary);
    std::ifstream b(right, std::ios::binary);
    std::string aa(1024 * 1024, '\0');
    std::string bb(1024 * 1024, '\0');
    do {
        a.read(aa.data(), aa.size());
        b.read(bb.data(), bb.size());
        ASSERT_EQ(a.gcount(), b.gcount());
        ASSERT_EQ(std::string_view(aa.data(), static_cast<std::size_t>(a.gcount())),
                  std::string_view(bb.data(), static_cast<std::size_t>(b.gcount())));
    } while (a.gcount() != 0);
}

class SingleWorkerTest : public ::testing::TestWithParam<std::uint64_t> {
  protected:
    void SetUp() override {
        root = std::filesystem::temp_directory_path() /
               ("storage-integration-" + std::to_string(reinterpret_cast<std::uintptr_t>(this)));
        std::filesystem::create_directories(root);
        store = std::make_unique<FileChunkStore>(root / "chunks");
        data_plane = std::make_unique<DataPlaneService>(*store);
        service = std::make_unique<WorkerService>(*data_plane);
        grpc::ServerBuilder builder;
        builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
        builder.RegisterService(service.get());
        server = builder.BuildAndStart();
        ASSERT_NE(server, nullptr);
    }
    void TearDown() override {
        if (server) {
            server->Shutdown();
            server->Wait();
        }
        std::filesystem::remove_all(root);
    }
    std::filesystem::path root;
    std::unique_ptr<FileChunkStore> store;
    std::unique_ptr<DataPlaneService> data_plane;
    std::unique_ptr<WorkerService> service;
    std::unique_ptr<grpc::Server> server;
    int port{0};
};

TEST_P(SingleWorkerTest, PutHeadGetDeleteOverGrpc) {
    const auto source = root / "source.bin";
    const auto destination = root / "destination.bin";
    WritePattern(source, GetParam());
    GrpcDataPlaneClient data_client;
    StorageClient client(data_client, WorkerEndpoint{"127.0.0.1:" + std::to_string(port)});
    const ObjectId id{"integration-" + std::to_string(GetParam())};
    ASSERT_TRUE(client.Put(id, source).ok());
    auto head = client.Head(id);
    ASSERT_TRUE(head.ok()) << head.status().message();
    EXPECT_EQ(head.value().size, GetParam());
    ASSERT_TRUE(client.Get(id, destination).ok());
    ExpectFilesEqual(source, destination);

    const auto existing = root / "existing.bin";
    WritePattern(existing, 1);
    EXPECT_EQ(client.Get(id, existing).code(), StatusCode::kAlreadyExists);
    EXPECT_EQ(std::filesystem::file_size(existing), 1);

    const auto chunk_file = *std::filesystem::directory_iterator(root / "chunks");
    std::fstream corrupt(chunk_file.path(), std::ios::binary | std::ios::in | std::ios::out);
    corrupt.seekp(80);
    corrupt.put('\x7f');
    corrupt.close();
    const auto corrupt_destination = root / "corrupt-download.bin";
    EXPECT_EQ(client.Get(id, corrupt_destination).code(), StatusCode::kCorruption);
    EXPECT_FALSE(std::filesystem::exists(corrupt_destination));

    ASSERT_TRUE(client.Delete(id).ok());
    EXPECT_EQ(client.Head(id).status().code(), StatusCode::kNotFound);
    EXPECT_EQ(client.Get(id, root / "missing.bin").code(), StatusCode::kNotFound);
}

INSTANTIATE_TEST_SUITE_P(Sizes, SingleWorkerTest,
                         ::testing::Values(1ULL * 1024 * 1024, 64ULL * 1024 * 1024));

} // namespace
} // namespace storage
