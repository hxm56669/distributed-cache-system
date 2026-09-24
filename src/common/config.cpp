#include "storage/common/config.h"

#include <string_view>

namespace storage {
namespace {

StatusOr<std::string> ReadOption(int argc, char *argv[], std::string_view option,
                                 std::string default_value) {
    for (int index = 1; index < argc; ++index) {
        const std::string_view argument(argv[index]);
        const std::string prefix = std::string(option) + "=";
        if (argument.starts_with(prefix)) {
            const auto value = std::string(argument.substr(prefix.size()));
            if (value.empty()) {
                return Status{StatusCode::kInvalidArgument,
                              std::string(option) + " cannot be empty"};
            }
            return value;
        }
        if (argument == option) {
            if (index + 1 >= argc || std::string_view(argv[index + 1]).empty()) {
                return Status{StatusCode::kInvalidArgument,
                              std::string(option) + " requires a value"};
            }
            return std::string(argv[index + 1]);
        }
    }
    return default_value;
}

} // namespace

StatusOr<ControlConfig> LoadControlConfig(int argc, char *argv[]) {
    auto address = ReadOption(argc, argv, "--listen", "127.0.0.1:50051");
    if (!address.ok()) {
        return address.status();
    }
    return ControlConfig{std::move(address).value()};
}

StatusOr<WorkerConfig> LoadWorkerConfig(int argc, char *argv[]) {
    auto address = ReadOption(argc, argv, "--listen", "127.0.0.1:50052");
    if (!address.ok()) {
        return address.status();
    }
    auto worker_id = ReadOption(argc, argv, "--worker-id", "worker-v0");
    if (!worker_id.ok()) {
        return worker_id.status();
    }
    return WorkerConfig{WorkerId{std::move(worker_id).value()}, std::move(address).value()};
}

StatusOr<ClientConfig> LoadClientConfig(int argc, char *argv[]) {
    auto control = ReadOption(argc, argv, "--control", "127.0.0.1:50051");
    if (!control.ok()) {
        return control.status();
    }
    auto worker = ReadOption(argc, argv, "--worker", "127.0.0.1:50052");
    if (!worker.ok()) {
        return worker.status();
    }
    return ClientConfig{std::move(control).value(), std::move(worker).value()};
}

} // namespace storage
