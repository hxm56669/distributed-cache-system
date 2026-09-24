# 03_第三方库与 vcpkg 基线清单

> 文档性质：本项目第三方 C/C++ 依赖、外部服务和系统工具的统一依赖基线。  
> 目标：保证构建可复现、依赖职责清晰，避免后续开发阶段随意引入重复或功能重叠的库。  
> 配套文档：
>
> - `AGENTS.md`
> - `01_核心类型与接口契约.md`
> - `02_V0-V11开发蓝图.md`

---

# 1. vcpkg 基线

本项目固定使用：

```text
vcpkg release:
2026.07.29

builtin-baseline:
9e593bb18ea69cc5095e012465dcd675a822ed0d
```

GitHub Release：

```text
https://github.com/microsoft/vcpkg/releases/tag/2026.07.29
```

Commit：

```text
https://github.com/microsoft/vcpkg/commit/9e593bb18ea69cc5095e012465dcd675a822ed0d
```

## 1.1 为什么选择这个基线

选择原则：

```text
官方 Release
>
固定 Commit
>
当前所需 Ports 已存在
>
避免跟随 master 漂移
```

不直接跟随 vcpkg `master`，原因：

- master 每天都会变化；
- 同一个 `vcpkg.json` 在不同日期可能解析到不同 Port 版本；
- 构建失败时难以确定是项目代码变化还是 Port 更新；
- Benchmark 期间依赖变化会污染性能对比；
- 求职项目需要能够复现历史版本。

因此：

> **除非明确进行依赖升级，不修改 builtin-baseline。**

依赖升级必须作为独立变更处理：

```text
build: update vcpkg baseline
```

并重新执行：

```text
Debug build
Sanitizer
Unit Test
Integration Test
必要 Benchmark
```

---

# 2. 基础 vcpkg.json 形式

项目采用：

```text
vcpkg Manifest Mode
```

根目录维护：

```text
vcpkg.json
```

基础结构：

```json
{
  "name": "<project-name>",
  "version-string": "0.0.0",
  "builtin-baseline": "9e593bb18ea69cc5095e012465dcd675a822ed0d",
  "dependencies": []
}
```

原则：

> 不一次性把 V0～V11 所有依赖加入 `vcpkg.json`。

而是：

```text
进入哪个阶段
→ 再加入哪个阶段真正需要的库
```

这样能减少：

- 编译时间；
- 二进制体积；
- 依赖冲突；
- 无意义的传递依赖；
- 项目早期复杂度。

---

# 3. 直接依赖总表

| 库 | vcpkg Port | 基线版本 | 首次引入阶段 | 定位 |
|---|---|---:|---:|---|
| spdlog | `spdlog` | 1.17.0#1 | V0 | 日志 |
| GoogleTest | `gtest` | 1.17.0#3 | V0 | 单元/集成测试 |
| Protocol Buffers | `protobuf` | 6.33.4#2 | V0 | 控制协议序列化 |
| gRPC | `grpc` | 1.81.1 | V0 | 控制面 RPC |
| BLAKE3 | `blake3` | 1.8.5 | V0 | 数据完整性校验 |
| CLI11 | `cli11` | 2.6.2 | V1 | CLI |
| SQLite | `sqlite3` | 3.53.4 | V5/V6 后 | Control Plane 本地持久化 |
| Google Benchmark | `benchmark` | 1.9.5 | V3+ 可选 | 微基准 |
| liburing | `liburing` | 2.15 | V7 | io_uring |
| Drogon | `drogon` | 1.9.13#1 | V8 | C++ HTTP API |
| MySQL Connector/C++ | `mysql-connector-cpp` | 9.7.0 | V8 | MySQL 客户端 |
| redis-plus-plus | `redis-plus-plus` | 1.3.15 | V8 | Redis C++ 客户端 |
| librdkafka | `librdkafka` | 2.14.2 | V9 | Kafka 客户端 |
| minio-cpp | `minio-cpp` | 0.4.0 | V3/V10 | S3/MinIO Backend 客户端 |
| prometheus-cpp | `prometheus-cpp` | 1.3.0 | V10 | Metrics |

说明：

- 表中的版本为当前固定 `builtin-baseline` 下的 Port 版本；
- `#N` 是 vcpkg Port Revision，不是上游项目版本的一部分；
- 后续不单独手写版本号覆盖，优先让 baseline 统一解析；
- 如果某个库未来因为兼容性必须 override，应单独记录原因。

---

# 4. V0 —— spdlog

vcpkg：

```text
spdlog
```

基线：

```text
1.17.0#1
```

用途：

```text
Control Plane 日志
Worker 日志
Client 日志
错误上下文
性能阶段可控日志等级
```

选择理由：

1. C++ 项目成熟度高；
2. 接口简单；
3. 支持同步/异步日志；
4. 支持格式化；
5. 容易按模块创建 logger；
6. 在高频 Data Plane 中可以关闭 INFO；
7. 已经适合本项目现有 C++ 工程习惯。

不使用：

```text
std::cout
```

作为正式运行日志。

## 4.1 fmt

`spdlog` 在此基线默认启用：

```text
fmt
```

因此 `fmt` 会作为传递依赖进入构建。

本项目当前：

> **不单独把 `fmt` 写进直接 dependencies。**

原因：

- 当前没有独立于 spdlog 的强需求；
- C++20 已提供越来越多格式化能力；
- 避免人为维护重复直接依赖。

如果未来大量非日志代码明确依赖 fmt，再提升为直接依赖。

---

# 5. V0 —— GoogleTest

vcpkg：

```text
gtest
```

基线：

```text
1.17.0#3
```

用途：

```text
Unit Test
Integration Test
Failure Test
部分 E2E Test Harness
```

选择理由：

- C++ 测试事实标准之一；
- 与 CMake/CTest 集成成熟；
- 支持断言、Fixture、Parameterized Test；
- 适合后续测试复杂状态机；
- vcpkg 支持稳定。

不选择 Catch2，原因不是其能力不足，而是：

> 项目只需要一套测试框架，GoogleTest 更符合当前项目和既有开发习惯。

---

# 6. V0 —— Protocol Buffers

vcpkg：

```text
protobuf
```

基线：

```text
6.33.4#2
```

用途：

```text
Control Plane RPC Message
Worker Register
Heartbeat
StartPut
CommitPut
ReadPlan
状态与错误码传输
```

选择理由：

1. Schema 明确；
2. 向后兼容机制成熟；
3. 与 gRPC 原生配套；
4. 比手写 JSON RPC 更适合控制面；
5. 二进制体积和序列化性能适合频繁元数据请求。

约束：

> protobuf Message 不等同于 Domain Model。

采用：

```text
proto message
    ↓ adapter
domain type
    ↓
service
```

避免核心代码被 RPC 框架绑死。

---

# 7. V0 —— gRPC

vcpkg：

```text
grpc
```

基线：

```text
1.81.1
```

用途：

```text
Control Plane RPC

Worker Register
Heartbeat

StartPut
CommitPut
GetReadPlan

Health Check
管理接口
```

选择理由：

- protobuf 原生集成；
- Codegen 成熟；
- 支持 Unary / Streaming；
- 有成熟 deadline / status / channel；
- 适合低流量控制面；
- 能快速把分布式控制协议做正确。

## 7.1 gRPC 不承担什么

最终原则：

```text
Control Plane
→ gRPC

Data Plane
→ 可以从 gRPC Stream 起步
→ 后续独立 TCP / sendfile / io_uring 优化
```

不把 gRPC 永久绑定为高性能大对象 Data Plane。

原因：

> 本项目要研究数据路径自身，而不是把所有网络细节永久交给 RPC 框架。

---

# 8. V0 —— BLAKE3

vcpkg：

```text
blake3
```

基线：

```text
1.8.5
```

用途：

```text
Object checksum
Chunk checksum
Put 校验
Get 校验
副本修复校验
数据损坏检测
```

选择理由：

- 高吞吐；
- 原生支持 SIMD；
- 适合大文件；
- 增量 Hash；
- 适合与 IO Pipeline 结合；
- 比 SHA-256 更适合作为本项目高性能完整性校验基线。

当前不启用：

```text
blake3[tbb]
```

原因：

> 不让 Hash 库自己隐式开启另一套线程调度，后续并行由项目自己的 Pipeline 控制。

---

# 9. V1 —— CLI11

vcpkg：

```text
cli11
```

基线：

```text
2.6.2
```

用途：

```text
cachectl
control-plane flags
worker flags
benchmark flags
```

例如：

```bash
cachectl push ...
cachectl pull ...
cachectl prefetch ...
cachectl nodes
```

选择理由：

- Header 风格使用简单；
- 子命令和 Option 支持完整；
- 不值得自行编写命令行 Parser；
- 依赖非常轻；
- 与 C++ CLI 工具非常匹配。

---

# 10. V5/V6 后 —— SQLite

vcpkg：

```text
sqlite3
```

基线：

```text
3.53.4
```

用途：

> 单 Control Plane 原型的本地 Metadata 持久化。

保存：

```text
Object Metadata
Chunk Metadata
Replica Metadata
Worker Metadata（必要持久部分）
Generation
Commit State
```

选择理由：

1. 单文件；
2. 有事务；
3. Crash Recovery 成熟；
4. 非常适合当前单 Master 原型；
5. 可以明确实现：
   `WRITING → READY` 原子提交；
6. 不需要提前引入分布式数据库。

注意：

> SQLite 不用于大对象数据本身。

也不意味着未来工业系统一定使用 SQLite。

本项目使用它解决：

```text
单机控制面持久化
+
Crash Recovery
```

而不是假装解决分布式一致性。

---

# 11. V3+ 可选 —— Google Benchmark

vcpkg：

```text
benchmark
```

基线：

```text
1.9.5
```

用途：

```text
Placement Algorithm Microbenchmark
LRU / SLRU Lookup
ReadPlan Build
Metadata Lookup
Checksum Microbenchmark
```

选择理由：

- 对小函数/算法微基准成熟；
- 支持重复、统计和防优化；
- 比自己写 `chrono` 循环更可靠。

## 11.1 不是系统 Benchmark

项目仍然保留自己的：

```text
bench_client
workload generator
E2E benchmark
```

Google Benchmark 只回答：

> 某个局部函数有多快？

它不能代替：

```text
系统吞吐
P99
多 Worker
故障恢复
Backend Traffic
```

因此是：

```text
可选直接依赖
```

而不是 V0 必须依赖。

---

# 12. V7 —— liburing

vcpkg：

```text
liburing
```

基线：

```text
2.15
```

平台：

```text
Linux only
```

用途：

```text
io_uring Data Plane 实验
Async Read
Async Write
Batch Completion
IO Pipeline
```

选择理由：

- Linux 官方生态最常用 io_uring 用户态库；
- 比直接手写 syscall ring 操作安全；
- 保留对 CQE/SQE 等原生机制的理解；
- 与项目 Linux Storage 定位一致。

重要规则：

> io_uring 是 V7 的性能实验，不是项目 V0 的架构依赖。

必须：

```text
Sync IO Baseline
vs
io_uring

真实 Benchmark
```

有收益才保留。

---

# 13. V8 —— Drogon

vcpkg：

```text
drogon
```

基线：

```text
1.9.13#1
```

用途：

```text
Artifact Registry HTTP API
REST Routing
Request / Response
Middleware
API Server
```

选择理由：

1. 原生 C++ Web Framework；
2. Linux 服务端使用成熟；
3. 支持 C++17/20；
4. HTTP、路由、异步处理已经完整；
5. 不需要在业务层重新发明 HTTP Parser；
6. 项目底层 Data Plane 已经会体现 socket / IO 能力，业务层没必要重复写 HTTP 基础设施。

## 13.1 为什么不选择 Boost.Beast

Boost.Beast 更底层，能展示 HTTP / Asio 细节。

但本项目已经有：

```text
Data Plane
TCP
IO
并发
Buffer
```

需要自己深入。

如果业务层再使用 Beast 自建 Router / Middleware，会大量消耗时间，却不会增加 Storage Core 深度。

因此选择：

```text
Storage / Network Core
→ 自己做深

Business HTTP
→ Drogon
```

---

# 14. V8 —— MySQL Connector/C++

vcpkg：

```text
mysql-connector-cpp
```

基线：

```text
9.7.0
```

用途：

```text
Project
Artifact
ArtifactVersion
Alias
Tag
UploadSession
PrefetchJob
Audit
Outbox
```

选择理由：

- 官方 MySQL C++ Connector；
- 能直接体现 C++ 数据库客户端使用；
- 支持事务；
- 业务 Repository 不绑在 Drogon ORM 上；
- 后续可以清楚讨论：
  连接池、事务、索引、隔离级别、Prepared Statement。

## 14.1 为什么不用 Drogon ORM 替代

Drogon 自身有 ORM/MySQL Feature。

本项目刻意不使用它作为主要数据库抽象，原因：

> 希望业务 Repository 与 HTTP Framework 解耦。

形成：

```text
Drogon
→ HTTP

mysql-connector-cpp
→ Database
```

即使未来替换 HTTP Framework，Repository 不需要重写。

---

# 15. V8 —— redis-plus-plus

vcpkg：

```text
redis-plus-plus
```

基线：

```text
1.3.15
```

底层传递依赖：

```text
hiredis
```

用途：

```text
Idempotency Key
Rate Limit
短期 Job State
Session / Token Cache
Hot Business Metadata
```

选择理由：

- 现代 C++ Redis API；
- 基于成熟 hiredis；
- RAII 风格；
- 不需要自己封装 hiredis C API；
- API 清晰，适合业务层。

当前不启用：

```text
async
tls
```

除非 V8 实际需求证明需要。

避免额外引入：

```text
libuv
```

---

# 16. V9 —— librdkafka

vcpkg：

```text
librdkafka
```

基线：

```text
2.14.2
```

用途：

```text
ArtifactUploadCompleted
PrefetchRequested
PrefetchCompleted
Audit Event
GC Event
```

以及：

```text
Outbox Relay
Consumer Group
Retry
Consumer Lag
```

选择理由：

- Kafka C/C++ 最成熟客户端之一；
- 生产使用广泛；
- 功能完整；
- 支持 Producer / Consumer / Group；
- 能直接学习 Kafka 的 offset、delivery report、rebalance 等真实机制。

## 16.1 为什么不选 cppkafka

`cppkafka` 是 librdkafka 上层 C++ Wrapper。

本项目不选择它作为直接依赖，主要原因：

- 又增加一层抽象；
- 额外引入 Boost Program Options；
- 我们本来就需要理解 Kafka Producer/Consumer 语义；
- librdkafka 自身已提供 C/C++ 接口。

所以：

```text
librdkafka
→ 直接使用
```

---

# 17. V3/V10 —— minio-cpp

vcpkg：

```text
minio-cpp
```

基线：

```text
0.4.0
```

用途：

```text
MinIO / S3 Compatible Backend

Cache Miss 回源
Backend Put
Backend Get
Backend Head
Backend Delete
```

选择理由：

- 专门面向 S3 Compatible Object Storage；
- MinIO 本身就是项目计划使用的 Backend；
- 比 AWS SDK for C++ 轻；
- 项目只需要 S3 Object 基础能力；
- 不需要整个 AWS 服务生态。

## 17.1 为什么不选 AWS SDK for C++

AWS SDK 更完整，但非常庞大。

本项目只需要：

```text
S3-Compatible Object API
```

因此：

```text
minio-cpp
```

更符合：

```text
最小依赖
+
清晰职责
```

如果未来真的接 AWS S3 且遇到兼容需求，再评估 AWS SDK。

---

# 18. V10 —— prometheus-cpp

vcpkg：

```text
prometheus-cpp
```

基线：

```text
1.3.0
```

用途：

```text
Counter
Gauge
Histogram
```

指标：

```text
storage_get_total
storage_put_total

request_duration

ram_cache_hit_total
nvme_cache_hit_total
backend_miss_total

backend_bytes_read

worker_capacity
worker_used

repair_total

readplan_cache_hit_total

job_queue_depth
outbox_pending
```

选择理由：

- C++ Prometheus Client；
- Metrics 类型清晰；
- 直接和 Prometheus/Grafana 配合；
- 不需要自己实现 Metrics Registry。

推荐：

```text
Pull 模式
```

不使用 Push Gateway 作为默认方案。

---

# 19. 不作为直接依赖的传递库

这些库很可能出现在最终 vcpkg dependency tree 中，但当前不应该全部手写进 `dependencies`。

## fmt

来源：

```text
spdlog
```

用途：

```text
Formatting
```

---

## OpenSSL

来源可能包括：

```text
gRPC
mysql-connector-cpp
minio-cpp
```

用途：

```text
TLS / Crypto transport dependency
```

项目当前不直接调用 OpenSSL API，因此不直接声明。

---

## hiredis

来源：

```text
redis-plus-plus
```

项目直接使用 redis-plus-plus。

---

## nlohmann-json

来源：

```text
minio-cpp
```

业务 HTTP JSON 默认使用 Drogon 自身 JSON 能力。

因此不为了“大家都用”再额外直接依赖 nlohmann-json。

---

## jsoncpp / trantor

来源：

```text
Drogon
```

属于 Drogon 内部核心依赖。

---

## abseil / c-ares / re2 / zlib / utf8-range

来源：

```text
gRPC / protobuf
```

属于 RPC 依赖树。

---

## curlpp / pugixml / inih

来源：

```text
minio-cpp
```

不直接使用。

---

# 20. 当前明确不引入的库

为了控制项目边界，默认不引入：

```text
Boost.Asio
Boost.Beast

RocksDB

etcd Client

Raft Library

ZooKeeper Client

Folly

TBB

libevent

libuv

OpenTelemetry C++

MongoDB Driver

RabbitMQ Client

Poco

AWS SDK for C++
```

说明：

这不是说这些库不好，而是当前架构已经有对应解决方案。

例如：

```text
HTTP
→ Drogon

RPC
→ gRPC

Redis
→ redis-plus-plus

Kafka
→ librdkafka

S3
→ minio-cpp

Local Metadata
→ SQLite

Async Linux IO
→ liburing
```

不要重复堆技术。

---

# 21. V0 最小 vcpkg.json

V0 只建议加入：

```json
{
  "name": "<project-name>",
  "version-string": "0.0.0",
  "builtin-baseline": "9e593bb18ea69cc5095e012465dcd675a822ed0d",
  "dependencies": [
    "blake3",
    "grpc",
    "gtest",
    "protobuf",
    "spdlog"
  ]
}
```

注意：

> CLI11 可以到 V1 再加入。

这能够让 V0 保持真正最小。

---

# 22. V1 增量

V1 加：

```json
"cli11"
```

用于：

```text
client
worker
control
cachectl
```

的参数与子命令。

---

# 23. V3 增量

如果此时正式接入 Backend MinIO：

```json
"minio-cpp"
```

微基准需要时：

```json
"benchmark"
```

---

# 24. V5/V6 持久化增量

Control Plane 开始做 Crash Recovery 时：

```json
"sqlite3"
```

---

# 25. V7 增量

Linux IO Optimization：

```json
{
  "name": "liburing",
  "platform": "linux"
}
```

必须保证：

```text
非 Linux 平台不强制安装
```

虽然项目正式目标就是 Linux，但 Manifest 仍尽量表达清楚平台限制。

---

# 26. V8 增量

业务平面增加：

```json
"drogon",
"mysql-connector-cpp",
"redis-plus-plus"
```

不启用 Drogon 自带 MySQL / Redis Features。

原因：

```text
HTTP Framework
Database Client
Redis Client
```

三者保持独立。

---

# 27. V9 增量

```json
"librdkafka"
```

第一阶段先使用基础 Producer/Consumer。

是否启用：

```text
ssl
zstd
sasl
```

根据实际 Kafka 部署需求再决定，不默认全部开启。

---

# 28. V10 增量

```json
"prometheus-cpp"
```

如果默认 feature 已包含：

```text
pull
compression
```

则不再重复写 feature。

---

# 29. 外部 Docker 服务

下面这些不是 vcpkg C++ 库。

最终通过：

```text
docker compose
```

管理。

| 组件 | 阶段 | 用途 |
|---|---:|---|
| MySQL Server | V8 | 业务元数据 |
| Redis Server | V8 | 幂等、限流、短期状态 |
| Kafka Broker | V9 | 事件与异步 Job |
| MinIO | V3/V10 | S3-Compatible Backend |
| Nginx | V10 | Reverse Proxy / TLS / 限流 |
| Prometheus | V10 | Metrics Scrape |
| Grafana | V10 | Dashboard |

原则：

```text
C++ Client Library
→ vcpkg

Server / Infrastructure
→ Docker Compose
```

---

# 30. 系统级工具

以下不是 C++ Library，不进 vcpkg。

通过 Ubuntu：

```text
apt
```

安装。

## Build

```text
gcc / g++
clang
cmake
ninja-build
git
pkg-config
```

## Debug

```text
gdb
```

## Performance

```text
perf
strace
sysstat
```

其中 `sysstat` 提供：

```text
iostat
pidstat
```

## Deployment

```text
docker
docker compose
```

## Network Debug

推荐：

```text
curl
tcpdump
ss
ip
```

---

# 31. Sanitizer

Sanitizer 不属于第三方库。

直接使用编译器：

```text
ASan
UBSan
```

可选后续：

```text
TSan
```

但 TSan 不与 ASan 同时运行。

---

# 32. Clang 工具

同样不走 vcpkg：

```text
clang-format
clang-tidy
```

作用：

```text
格式统一
静态分析
```

---

# 33. 依赖引入审核规则

未来任何新库加入前必须回答五个问题：

```text
1. 解决什么具体问题？

2. 标准库能不能解决？

3. 当前已有库能不能解决？

4. 是否进入性能关键路径？

5. 是否值得增加新的长期维护依赖？
```

如果回答只是：

> “这个库很流行。”

不能作为引入理由。

---

# 34. 直接依赖控制原则

核心原则：

> 项目难点必须由我们自己的代码承担。

第三方库负责：

```text
Logging
Testing
RPC
Serialization
Hash
CLI
Database Client
Redis Client
Kafka Client
S3 Client
Metrics
```

自己实现：

```text
Object / Chunk / Replica

Metadata Model

UploadPlan
ReadPlan

Placement

Replication

Failure Detection

Repair

Tiered Cache

Eviction / Admission

Prefetch

Hedged Read

Backpressure

Data Plane

IO Optimization

StorageClient Coordination
```

这样项目不会退化成：

> “把几个库连接起来。”

---

# 35. 依赖分层

最终依赖关系建议：

```text
                     Business
                        │
           ┌────────────┼─────────────┐
           ▼            ▼             ▼
        Drogon      MySQL Client    Redis Client
           │
           └──────── StorageClient
                        │
================================================
                    Storage Core
================================================
                        │
        ┌───────────────┼────────────────┐
        ▼               ▼                ▼
      gRPC           protobuf          BLAKE3
        │
        │
     Data Plane
        │
   ┌────┼───────────────┐
   ▼    ▼               ▼
POSIX  liburing      minio-cpp
 IO                   Backend
================================================
                    Event Layer
================================================
                    librdkafka

================================================
                  Observability
================================================
                   spdlog
               prometheus-cpp
```

---

# 36. 推荐最终依赖策略

不是：

```text
第一天安装所有依赖
```

而是：

```text
V0
spdlog
gtest
protobuf
grpc
blake3

V1
+ cli11

V3
+ minio-cpp
+ benchmark（需要时）

V5/V6
+ sqlite3

V7
+ liburing

V8
+ drogon
+ mysql-connector-cpp
+ redis-plus-plus

V9
+ librdkafka

V10
+ prometheus-cpp
```

这样每次 `git diff` 都能明确回答：

> 为什么这次多了这个库？

---

# 37. vcpkg Baseline 更新规则

正常开发期间：

```text
禁止自动 update baseline
```

只有出现：

```text
安全问题
严重 Bug
新库在旧 baseline 不存在
编译器兼容问题
明确需要新 Feature
```

才考虑升级。

升级流程：

```text
新 branch
    ↓
更新 vcpkg baseline
    ↓
重新 install
    ↓
Debug
    ↓
Sanitizer
    ↓
Full Test
    ↓
关键 Benchmark A/B
    ↓
确认
    ↓
独立 Commit
```

不允许：

```text
功能修改
+
baseline 更新
```

混在一个 Commit。

---

# 38. 当前最终选择

本项目第三方依赖基线最终确定为：

```text
vcpkg 2026.07.29
commit:
9e593bb18ea69cc5095e012465dcd675a822ed0d
```

核心直接依赖：

```text
spdlog
gtest
protobuf
grpc
blake3
cli11

sqlite3
benchmark
liburing

drogon
mysql-connector-cpp
redis-plus-plus

librdkafka
minio-cpp
prometheus-cpp
```

其中：

```text
V0 实际只安装 5 个核心库
```

其他依赖按照阶段逐步进入。

这套选择的原则是：

```text
成熟
稳定
职责单一
Linux/C++ 生态常见
能通过 vcpkg 固定版本
不重复解决同一问题
不抢走自研 Storage Core 的技术深度
```
