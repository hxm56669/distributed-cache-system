#include "storage/rpc/services.h"

#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <gtest/gtest.h>

#include <memory>
#include <string>

namespace storage {
namespace {

template <typename Service> struct RunningServer {
    Service service;
    std::unique_ptr<grpc::Server> server;
    int port{0};

    RunningServer() {
        grpc::ServerBuilder builder;
        builder.AddListeningPort("127.0.0.1:0", grpc::InsecureServerCredentials(), &port);
        builder.RegisterService(&service);
        server = builder.BuildAndStart();
    }

    ~RunningServer() {
        if (server) {
            server->Shutdown();
            server->Wait();
        }
    }
};

void CheckControlHealth(int port) {
    auto stub = rpc::ControlPlaneRpc::NewStub(grpc::CreateChannel(
        "127.0.0.1:" + std::to_string(port), grpc::InsecureChannelCredentials()));
    grpc::ClientContext context;
    rpc::HealthRequest request;
    rpc::HealthResponse response;
    const auto status = stub->Health(&context, request, &response);
    ASSERT_TRUE(status.ok()) << status.error_message();
    EXPECT_EQ(response.status().code(), rpc::STATUS_CODE_OK);
    EXPECT_EQ(response.service(), "control-plane");
}

void CheckWorkerHealth(int port) {
    auto stub = rpc::WorkerRpc::NewStub(grpc::CreateChannel("127.0.0.1:" + std::to_string(port),
                                                            grpc::InsecureChannelCredentials()));
    grpc::ClientContext context;
    rpc::HealthRequest request;
    rpc::HealthResponse response;
    const auto status = stub->Health(&context, request, &response);
    ASSERT_TRUE(status.ok()) << status.error_message();
    EXPECT_EQ(response.status().code(), rpc::STATUS_CODE_OK);
    EXPECT_EQ(response.service(), "worker");
}

TEST(RpcSmokeTest, HealthTraversesGrpcForBothServices) {
    RunningServer<ControlPlaneService> control;
    RunningServer<WorkerService> worker;
    ASSERT_NE(control.server, nullptr);
    ASSERT_NE(worker.server, nullptr);
    ASSERT_GT(control.port, 0);
    ASSERT_GT(worker.port, 0);

    CheckControlHealth(control.port);
    CheckWorkerHealth(worker.port);
}

} // namespace
} // namespace storage
