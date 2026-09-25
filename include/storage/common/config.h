#pragma once

#include "storage/common/status_or.h"
#include "storage/storage/types/worker_id.h"

#include <string>
#include <filesystem>

namespace storage {

struct ControlConfig {
    std::string listen_address{"127.0.0.1:50051"};
};

struct WorkerConfig {
    WorkerId worker_id{"worker-v0"};
    std::string listen_address{"127.0.0.1:50052"};
    std::filesystem::path storage_root{"./worker-data"};
};

struct ClientConfig {
    std::string control_address{"127.0.0.1:50051"};
    std::string worker_address{"127.0.0.1:50052"};
};

StatusOr<ControlConfig> LoadControlConfig(int argc, char *argv[]);
StatusOr<WorkerConfig> LoadWorkerConfig(int argc, char *argv[]);
StatusOr<ClientConfig> LoadClientConfig(int argc, char *argv[]);

} // namespace storage
