#include "storage/storage/client/data_plane_client.h"
#include "storage/storage/client/storage_client.h"

#include <CLI/CLI.hpp>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <string>

namespace {
int Report(const storage::Status &status) {
    if (status.ok()) {
        return 0;
    }
    std::cerr << "error: " << status.message() << '\n';
    return 1;
}
} // namespace

int main(int argc, char *argv[]) {
    CLI::App app{"V1 single-worker storage client"};
    std::string worker_host{"127.0.0.1"};
    std::uint16_t worker_control_port{50052};
    std::uint16_t worker_data_port{50052};
    app.add_option("--worker-host", worker_host, "Worker host");
    app.add_option("--worker-control-port", worker_control_port, "Worker control port");
    app.add_option("--worker-data-port", worker_data_port, "Worker data port");

    std::string object_id;
    std::string path;
    auto *put = app.add_subcommand("put", "Store an immutable object");
    put->add_option("object-id", object_id)->required();
    put->add_option("source", path)->required();
    auto *get = app.add_subcommand("get", "Download an object");
    get->add_option("object-id", object_id)->required();
    get->add_option("destination", path)->required();
    auto *head = app.add_subcommand("head", "Show object metadata");
    head->add_option("object-id", object_id)->required();
    auto *remove = app.add_subcommand("delete", "Delete an object");
    remove->add_option("object-id", object_id)->required();
    app.require_subcommand(1);
    CLI11_PARSE(app, argc, argv);

    storage::GrpcDataPlaneClient data_plane(std::chrono::seconds(3600));
    storage::StorageClient client(
        data_plane, storage::WorkerEndpoint{worker_host, worker_control_port, worker_data_port});
    if (*put) {
        return Report(client.Put(storage::ObjectId{object_id}, path));
    }
    if (*get) {
        return Report(client.Get(storage::ObjectId{object_id}, path));
    }
    if (*remove) {
        return Report(client.Delete(storage::ObjectId{object_id}));
    }
    auto response = client.Head(storage::ObjectId{object_id});
    if (!response.ok()) {
        return Report(response.status());
    }
    std::cout << "object_id=" << object_id << " size=" << response.value().size
              << " checksum=" << response.value().checksum.value << '\n';
    return 0;
}
