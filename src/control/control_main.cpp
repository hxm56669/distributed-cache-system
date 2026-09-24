#include "storage/common/config.h"
#include "storage/common/logging.h"
#include "storage/rpc/services.h"

#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <spdlog/spdlog.h>

#include <memory>

int main(int argc, char *argv[]) {
    const auto log_status = storage::InitLogging({});
    if (!log_status.ok()) {
        return 1;
    }
    const auto config = storage::LoadControlConfig(argc, argv);
    if (!config.ok()) {
        spdlog::error("invalid control-plane configuration: {}", config.status().message());
        return 1;
    }

    storage::ControlPlaneService service;
    grpc::ServerBuilder builder;
    builder.AddListeningPort(config.value().listen_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    if (!server) {
        spdlog::error("failed to start control-plane on {}", config.value().listen_address);
        return 1;
    }
    spdlog::info("control-plane listening on {}", config.value().listen_address);
    server->Wait();
    return 0;
}
