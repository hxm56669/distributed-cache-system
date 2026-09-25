#include "storage/common/config.h"
#include "storage/common/logging.h"
#include "storage/rpc/services.h"
#include "storage/storage/worker/data_plane_service.h"
#include "storage/storage/worker/file_chunk_store.h"

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <spdlog/spdlog.h>

#include <memory>

int main(int argc, char *argv[]) {
    const auto log_status = storage::InitLogging({});
    if (!log_status.ok()) {
        return 1;
    }
    const auto config = storage::LoadWorkerConfig(argc, argv);
    if (!config.ok()) {
        spdlog::error("invalid worker configuration: {}", config.status().message());
        return 1;
    }

    storage::FileChunkStore chunk_store(config.value().storage_root);
    storage::DataPlaneService data_plane(chunk_store);
    storage::WorkerService service(data_plane);
    grpc::ServerBuilder builder;
    builder.AddListeningPort(config.value().listen_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    if (!server) {
        spdlog::error("failed to start worker on {}", config.value().listen_address);
        return 1;
    }
    spdlog::info("worker {} listening on {}", config.value().worker_id.value,
                 config.value().listen_address);
    spdlog::info("worker storage root: {}", config.value().storage_root.string());
    server->Wait();
    return 0;
}
