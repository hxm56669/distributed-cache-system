#include "storage/rpc/services.h"

namespace storage {
namespace {

void SetHealthy(rpc::HealthResponse *response, const char *service) {
    response->mutable_status()->set_code(rpc::STATUS_CODE_OK);
    response->mutable_status()->set_message("ok");
    response->set_service(service);
}

} // namespace

grpc::Status ControlPlaneService::Health(grpc::ServerContext *, const rpc::HealthRequest *,
                                         rpc::HealthResponse *response) {
    SetHealthy(response, "control-plane");
    return grpc::Status::OK;
}

grpc::Status WorkerService::Health(grpc::ServerContext *, const rpc::HealthRequest *,
                                   rpc::HealthResponse *response) {
    SetHealthy(response, "worker");
    return grpc::Status::OK;
}

} // namespace storage
