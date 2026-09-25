#pragma once

// 引入由 control.proto 经过 protoc + gRPC 插件自动生成的头文件。
// 里面会包含类似：
// rpc::ControlPlaneRpc
// rpc::ControlPlaneRpc::Service
// 等 gRPC 服务端相关类型。
#include "control.grpc.pb.h"
// 引入由 worker.proto 自动生成的 gRPC 头文件。
// 里面包含：
// rpc::WorkerRpc
// rpc::WorkerRpc::Service
// 等类型。
#include "worker.grpc.pb.h"
#include "storage/storage/worker/data_plane_service.h"

namespace storage {

  // 定义 ControlPlaneService 类。
  //
  // rpc::ControlPlaneRpc::Service：
  // 是根据 proto 中：
  //
  // service ControlPlaneRpc {
  //     rpc Health(...);
  // }
  //
  // 自动生成的服务端基类。
  //
  // 我们继承它，才能真正实现 ControlPlaneRpc 这个 gRPC 服务。
  //
  // final：
  // 表示不允许其他类继续继承 ControlPlaneService。
class ControlPlaneService final : public rpc::ControlPlaneRpc::Service {
  public:
        // context  当前这一次 RPC 请求的上下文。
        //
        // 里面可以获取：
        // - 客户端信息
        // - deadline
        // - metadata
        // - cancellation 状态
    grpc::Status Health(grpc::ServerContext *context, const rpc::HealthRequest *request,
                        rpc::HealthResponse *response) override;
};

class WorkerService final : public rpc::WorkerRpc::Service {
  public:
    WorkerService() = default;
    explicit WorkerService(DataPlaneService &data_plane) : data_plane_(&data_plane) {}
    grpc::Status Health(grpc::ServerContext *context, const rpc::HealthRequest *request,
                        rpc::HealthResponse *response) override;
    grpc::Status PutChunk(grpc::ServerContext *context,
                          grpc::ServerReader<rpc::PutChunkFrame> *reader,
                          rpc::OperationResponse *response) override;
    grpc::Status GetChunk(grpc::ServerContext *context, const rpc::GetChunkRequest *request,
                          grpc::ServerWriter<rpc::ChunkData> *writer) override;
    grpc::Status HeadChunk(grpc::ServerContext *context, const rpc::HeadChunkRequest *request,
                           rpc::HeadChunkResponse *response) override;
    grpc::Status DeleteChunk(grpc::ServerContext *context,
                             const rpc::DeleteChunkRequest *request,
                             rpc::OperationResponse *response) override;

  private:
    DataPlaneService *data_plane_{nullptr};
};

} // namespace storage
