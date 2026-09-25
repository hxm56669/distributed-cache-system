#include "storage/storage/client/data_plane_client.h"
#include "storage/storage/client/storage_client.h"

#include <chrono>
#include <iostream>
#include <string>
#include <string_view>

namespace {
int Report(const storage::Status &status) {
    if (status.ok())
        return 0;
    std::cerr << "error: " << status.message() << '\n';
    return 1;
}
} // namespace

int main(int argc, char *argv[]) {
    std::string worker_address{"127.0.0.1:50052"};
    int index = 1;
    if (index + 1 < argc && std::string_view(argv[index]) == "--worker") {
        worker_address = argv[index + 1];
        index += 2;
    }
    if (index >= argc) {
        std::cerr << "usage: client [--worker ADDRESS] put ID SOURCE | get ID DEST | head ID | "
                     "delete ID\n";
        return 2;
    }
    const std::string command = argv[index++];
    const int expected_arguments = (command == "put" || command == "get") ? 2 : 1;
    if ((command != "put" && command != "get" && command != "head" && command != "delete") ||
        argc - index != expected_arguments) {
        std::cerr << "invalid command or argument count\n";
        return 2;
    }
    const std::string object_id = argv[index++];

    storage::GrpcDataPlaneClient data_plane(std::chrono::seconds(3600));
    storage::StorageClient client(data_plane, storage::WorkerEndpoint{worker_address});
    if (command == "put")
        return Report(client.Put(storage::ObjectId{object_id}, argv[index]));
    if (command == "get")
        return Report(client.Get(storage::ObjectId{object_id}, argv[index]));
    if (command == "delete")
        return Report(client.Delete(storage::ObjectId{object_id}));
    auto response = client.Head(storage::ObjectId{object_id});
    if (!response.ok())
        return Report(response.status());
    std::cout << "object_id=" << object_id << " size=" << response.value().size
              << " checksum=" << response.value().checksum.value << '\n';
    return 0;
}
