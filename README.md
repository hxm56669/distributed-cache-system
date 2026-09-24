# Storage

Storage is a C++20 distributed, tiered-cache and data-placement project for large immutable
artifacts such as models, datasets, and checkpoints. The repository is currently at **V0: project
baseline**.

V0 supplies common types, process scaffolding, and real gRPC health communication among a control
plane, worker, and client. It intentionally does not implement Put/Get, chunk storage, ReadPlan,
placement, replication, cache tiers, or management-plane services.

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

Release presets are `release` and `bench-release`. The latter only validates the future benchmark
build configuration in V0; V0 contains no performance benchmark.

## Programs

```bash
./build/debug/control-plane
./build/debug/worker
./build/debug/client
```

The default endpoints are `127.0.0.1:50051` and `127.0.0.1:50052`. Addresses can be overridden by
`--listen`, `--control`, and `--worker`. The integration test performs genuine in-process-server,
loopback gRPC calls and can be run directly with:

```bash
ctest --test-dir build/debug -R RpcSmokeTest --output-on-failure
```
