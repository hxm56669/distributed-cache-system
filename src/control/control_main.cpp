#include "storage/common/config.h"
#include "storage/common/logging.h"
#include "storage/rpc/services.h"

// 引入 gRPC Server 类型。
// 后面 std::unique_ptr<grpc::Server> 中会使用 grpc::Server。
#include <grpcpp/server.h>
// 引入 gRPC ServerBuilder。
// ServerBuilder 负责配置监听地址、注册 Service，并最终启动 gRPC Server。
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


    // 创建 Control Plane 的 gRPC Service 对象。
    //
    // 这个对象负责真正处理客户端发送过来的 RPC 请求。
    //
    // 当前 V0 阶段主要是 Health / Ping 之类的最小 RPC。
    //
    // 后面 V2 才会逐步扩展：
    //
    // RegisterWorker
    // Heartbeat
    // StartPut
    // CommitPut
    // GetReadPlan
    //
    // 等真正的控制面逻辑。
    storage::ControlPlaneService service;
    // 创建 gRPC ServerBuilder。
    //
    // 可以把它理解成：
    //
    // “gRPC Server 的配置器 / 构造器”
    //
    // Server 还没有真正启动。
    grpc::ServerBuilder builder;

    // 为 gRPC Server 设置监听地址。
    // grpc::InsecureServerCredentials()
    // 表示当前不启用 TLS，加密认证暂时不处理
    builder.AddListeningPort(config.value().listen_address, grpc::InsecureServerCredentials());

    // 把刚才创建的 ControlPlaneService
    // 注册到这个 gRPC Server 中。
    // 以后客户端调用 Control Plane RPC 时，
    // gRPC Server 就会把请求分发到 service 对象。
    builder.RegisterService(&service);
    // 根据前面设置好的配置真正创建并启动 gRPC Server。
    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    if (!server) {
        spdlog::error("failed to start control-plane on {}", config.value().listen_address);
        return 1;
    }
    spdlog::info("control-plane listening on {}", config.value().listen_address);
    // 阻塞当前 main 线程，
    // 等待 gRPC Server 结束。
    server->Wait();
    return 0;
}
