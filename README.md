# Storage

Storage is a C++20 distributed, tiered-cache and data-placement project for large immutable
artifacts such as models, datasets, and checkpoints. The repository is currently at **V1: single
worker storage path**.

V1 adds an immutable, streaming Put/Get/Head/Delete path from the CLI through gRPC to an ext4-backed
worker. V1 intentionally remains one object per chunk and one configured worker; placement,
replication, ReadPlan, and cache tiers begin in later stages.

## Requirements

- Linux (Ubuntu 24.04 is the reference environment)
- GCC 13+ or Clang 17+
- CMake 4.4+, Ninja, Git (required by the pinned 2026 vcpkg baseline)
- vcpkg checked out at the baseline in `vcpkg.json`

Prepare the pinned vcpkg checkout once:

```bash
git clone https://github.com/microsoft/vcpkg.git third_party/vcpkg
git -C third_party/vcpkg checkout 9e593bb18ea69cc5095e012465dcd675a822ed0d
./third_party/vcpkg/bootstrap-vcpkg.sh -disableMetrics
```

## Build and test

```bash
cmake --preset debug
cmake --build --preset debug -j"$(nproc)"
ctest --preset debug --output-on-failure

cmake --preset sanitized
cmake --build --preset sanitized -j"$(nproc)"
ctest --preset sanitized --output-on-failure
```

The sanitized test preset disables ASan's optional user-poisoning API because the pinned gRPC
release uses manual stack poisoning internally during server shutdown. Address and undefined
behavior instrumentation remain enabled for all project targets.

Release presets are `release` and `bench-release`. Run the V1 baseline with:

```bash
./benchmarks/v1-single-worker/run.sh
```

## Programs

```bash
./build/debug/control-plane
./build/debug/worker --storage-root ./worker-data
./build/debug/client put demo-object ./source.bin
./build/debug/client head demo-object
./build/debug/client get demo-object ./download.bin
./build/debug/client delete demo-object
```

The worker defaults to `127.0.0.1:50052`; override it with worker `--listen` and client `--worker`.
The integration tests perform genuine loopback gRPC streaming for 1 MiB and 64 MiB files:

```bash
ctest --test-dir build/debug -R 'RpcSmokeTest|SingleWorkerTest' --output-on-failure
```

Chunk IDs are encoded before becoming filenames. A chunk is committed with no-replace semantics
only after its byte count and BLAKE3 checksum pass and its temporary file is synchronized. The
persisted file header keeps size and checksum available after a worker restart. Both disk and RPC
paths use fixed-size buffers; large objects are never loaded wholly into memory.
