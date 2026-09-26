#include "storage/common/config.h"

#include <CLI/CLI.hpp>

#include <string>

namespace storage {
namespace {

Status Parse(CLI::App &app, int argc, char *argv[]) {
    try {
        app.parse(argc, argv);
        return Status::Ok();
    } catch (const CLI::ParseError &error) {
        return {StatusCode::kInvalidArgument, error.what()};
    }
}

} // namespace

StatusOr<ControlConfig> LoadControlConfig(int argc, char *argv[]) {
    ControlConfig config;
    CLI::App app{"Storage control plane"};
    app.add_option("--listen", config.listen_address, "Control-plane listen address");
    if (auto status = Parse(app, argc, argv); !status.ok()) {
        return status;
    }
    return config;
}

StatusOr<WorkerConfig> LoadWorkerConfig(int argc, char *argv[]) {
    WorkerConfig config;
    CLI::App app{"Storage worker"};
    app.add_option("--listen", config.listen_address, "Worker listen address");
    app.add_option("--worker-id", config.worker_id.value, "Worker identifier");
    app.add_option("--storage-root", config.storage_root, "Chunk storage root");
    if (auto status = Parse(app, argc, argv); !status.ok()) {
        return status;
    }
    if (!config.worker_id.valid()) {
        return Status{StatusCode::kInvalidArgument, "--worker-id cannot be empty"};
    }
    return config;
}

StatusOr<ClientConfig> LoadClientConfig(int argc, char *argv[]) {
    ClientConfig config;
    CLI::App app{"Storage health client"};
    app.add_option("--control", config.control_address, "Control-plane endpoint");
    app.add_option("--worker", config.worker_address, "Worker endpoint");
    if (auto status = Parse(app, argc, argv); !status.ok()) {
        return status;
    }
    return config;
}

} // namespace storage
