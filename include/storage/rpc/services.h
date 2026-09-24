#pragma once

#include "control.grpc.pb.h"
#include "worker.grpc.pb.h"

namespace storage {

class ControlPlaneService final : public rpc::ControlPlaneRpc::Service {
  public:
    grpc::Status Health(grpc::ServerContext *context, const rpc::HealthRequest *request,
                        rpc::HealthResponse *response) override;
};

class WorkerService final : public rpc::WorkerRpc::Service {
  public:
    grpc::Status Health(grpc::ServerContext *context, const rpc::HealthRequest *request,
                        rpc::HealthResponse *response) override;
};

} // namespace storage
