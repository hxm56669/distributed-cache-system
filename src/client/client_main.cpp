#include "storage/common/config.h"
#include "storage/common/logging.h"

#include "control.grpc.pb.h"
#include "worker.grpc.pb.h"

#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>

namespace {

template <typename Stub> bool CheckHealth(Stub &stub, const std::string &name) {
    grpc::ClientContext context;
    context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(3));
    storage::rpc::HealthRequest request;
    storage::rpc::HealthResponse response;
    const auto status = stub.Health(&context, request, &response);
    if (!status.ok() || response.status().code() != storage::rpc::STATUS_CODE_OK) {
        std::cerr << name << " Health FAILED: " << status.error_message() << '\n';
        return false;
    }
    std::cout << name << " Health OK" << '\n';
    return true;
}

} // namespace

int main(int argc, char *argv[]) {
    if (!storage::InitLogging({}).ok()) {
        return 1;
    }
    const auto config = storage::LoadClientConfig(argc, argv);
    if (!config.ok()) {
        std::cerr << "invalid client configuration: " << config.status().message() << '\n';
        return 1;
    }
    auto control = storage::rpc::ControlPlaneRpc::NewStub(
        grpc::CreateChannel(config.value().control_address, grpc::InsecureChannelCredentials()));
    auto worker = storage::rpc::WorkerRpc::NewStub(
        grpc::CreateChannel(config.value().worker_address, grpc::InsecureChannelCredentials()));
    const bool control_ok = CheckHealth(*control, "Control");
    const bool worker_ok = CheckHealth(*worker, "Worker");
    return control_ok && worker_ok ? 0 : 1;
}
