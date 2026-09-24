# AGENTS.md

> 本文件约束本仓库中所有 AI 编程 Agent、代码生成工具和自动化修改行为。
>
> 优先级：当前用户明确要求 > 本文件 > 子目录更具体的 `AGENTS.md` > 现有代码与测试约定。
> 如有冲突，优先保持数据正确性、架构边界、已有测试行为和可审查性。

---

## 1. 项目定位

本项目是：

> **面向模型、数据集、Checkpoint 等大对象访问场景的 C++ 分布式分层缓存与数据放置系统。**

最终分为三个平面：

```text
业务管理平面
    ↓
存储控制平面
    ↓
存储数据平面
```

目标是做到：**可用、可部署、可验证、可 Benchmark**。

不得把项目退化为：

- 单机 LRU Demo；
- Master 代理全部大对象数据；
- 用 Redis 替代自研缓存；
- 用 MySQL 替代整个存储控制面；
- 为了堆技术引入无真实职责的组件。

---

## 2. 主开发环境

```text
Host: Windows 11
Guest: Ubuntu Linux VM
IDE: VS Code Remote SSH
Filesystem: ext4
Network: 桥接优先
Container: Ubuntu VM 内直接安装 Docker Engine + Docker Compose
```

建议 VM 资源：

```text
12 vCPU
20~24 GB RAM
约 220 GB 动态虚拟磁盘
```

正式编译、测试、性能分析均以 Linux 为准。Windows/MSVC 不作为主目标平台。

---

## 3. 语言与编译器

统一：

```text
C++20
```

CMake 必须设置：

```cmake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

主要支持：

```text
GCC >= 13
Clang >= 17
```

规则：

- 优先标准 C++20；
- Linux 专用能力放入 Linux 实现层；
- 不依赖编译器私有扩展解决通用问题；
- 不使用裸 `new/delete`；
- 资源一律 RAII；
- 所有权必须明确；
- 不为了炫技引入复杂模板元编程。

---

## 4. 包管理器

统一使用：

```text
vcpkg Manifest Mode
```

仓库必须存在：

```text
vcpkg.json
```

并固定：

```text
builtin-baseline
```

第三方 C/C++ 库优先通过 vcpkg 管理。

系统工具可以通过 apt 安装，例如：

```text
cmake
ninja-build
gcc/g++
clang
gdb
perf
strace
sysstat
docker
```

新增依赖前必须回答：

1. 标准库为什么不够？
2. 现有依赖为什么不够？
3. 它进入哪个模块？
4. 是否进入关键数据路径？

不要为很小功能引入大型库。

### vcpkg 固定方式

推荐继续使用：

```text
third_party/vcpkg
```

作为 git submodule，或使用外部固定 `VCPKG_ROOT`。

无论采用哪种方式，都必须保证：

```text
vcpkg.json
+ builtin-baseline
+ CMakePresets.json
```

可以复现构建。

更新 vcpkg baseline 必须单独提交，不与业务功能混在一起。

---

## 5. 构建系统

统一使用：

```text
CMake + Ninja + CMakePresets.json
```

不维护平行 Makefile 构建体系。

至少提供以下 Preset：

```text
debug
release
sanitized
bench-release
```

推荐命令：

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure

cmake --preset release
cmake --build --preset release

cmake --preset sanitized
cmake --build --preset sanitized
ctest --preset sanitized --output-on-failure

cmake --preset bench-release
cmake --build --preset bench-release
```

含义：

```text
debug        → 日常开发、单测
release      → 普通 Release
sanitized    → ASan + UBSan
bench-release→ 正式性能测试
```

---

## 6. 编译警告与 Sanitizer

项目自身 target 至少启用：

```text
-Wall
-Wextra
-Wpedantic
```

可逐步增加：

```text
-Wconversion
-Wshadow
-Wnon-virtual-dtor
```

禁止通过全局关闭 warning 来隐藏问题。

Sanitizer 至少支持：

```text
AddressSanitizer
UndefinedBehaviorSanitizer
```

正式 Benchmark 禁止使用 Sanitizer 构建结果。

---

## 7. 代码格式与静态分析

统一使用：

```text
clang-format
clang-tidy
```

仓库根目录必须有 `.clang-format`。

建议初始风格：

```text
BasedOnStyle: LLVM
IndentWidth: 4
ColumnLimit: 100
```

大括号风格在项目初始化时确定一次，之后不要擅自改变。

clang-tidy 优先启用：

```text
bugprone-*
performance-*
modernize-*
readability-*
```

不要一次性自动重写整个仓库。

---

## 8. 命名与文件规则

推荐统一：

```text
类型：PascalCase
函数：PascalCase
局部变量：snake_case
成员变量：snake_case_
常量：kPascalCase
文件：snake_case.h / snake_case.cpp
```

示例：

```cpp
class ReadPlanBuilder {
public:
    StatusOr<ReadPlan> Build(const ObjectId& object_id);

private:
    MetadataStore* metadata_store_;
};
```

公共接口放：

```text
include/<project_name>/...
```

实现放：

```text
src/...
```

规则：

- 头文件最小依赖；
- `.cpp` 首先 include 自己对应头文件；
- 不依赖“其他头碰巧帮我 include”；
- 不进行与当前任务无关的大规模格式化。

---

## 9. 错误模型

统一使用：

```text
Status
StatusOr<T>
```

建议错误码至少包含：

```cpp
enum class StatusCode {
    kOk,
    kInvalidArgument,
    kNotFound,
    kAlreadyExists,
    kUnavailable,
    kTimeout,
    kIoError,
    kCorruption,
    kResourceExhausted,
    kInternal,
};
```

不要混用 `int/bool/nullptr/异常/Status` 表达同类错误。

异常不作为普通业务控制流。

---

## 10. RAII 与 Linux 系统调用

所有资源必须明确生命周期，包括：

```text
fd
socket
mmap
thread
file
database connection
RPC channel
buffer
timer
```

fd 必须封装，例如：

```text
UniqueFd
```

Linux IO / socket 必须正确处理：

```text
EINTR
EAGAIN / EWOULDBLOCK
partial read
partial write
EOF
connection reset
SIGPIPE
EINPROGRESS
getsockopt(SO_ERROR)
```

不得假设一次 read/write 就完成全部数据。

---

## 11. 日志

统一使用：

```text
spdlog
```

规则：

- CLI 最终输出与运行日志分离；
- 高频数据路径默认不逐请求 INFO；
- ERROR 必须包含足够上下文；
- Benchmark 日志级别必须可配置；
- 禁止输出密钥、Token、密码。

推荐上下文：

```text
request_id
object_id
chunk_id
worker_id
latency
error
```

---

## 12. 测试

统一使用：

```text
GoogleTest + CTest
```

目录：

```text
tests/
├── unit/
├── integration/
├── failure/
└── e2e/
```

规则：

- 核心逻辑必须有 Unit Test；
- 跨模块协议必须有 Integration Test；
- 故障恢复必须有 Failure Test；
- 完整用户路径使用 E2E Test；
- 修复 Bug 时优先先增加复现测试。

“编译通过”不等于“功能完成”。

---

## 13. Benchmark 规则

测试回答“对不对”，Benchmark 回答“多快、瓶颈在哪”。

任何重要性能优化必须遵循：

```text
Baseline
→ Hypothesis
→ Implementation
→ Benchmark
→ perf / strace / iostat / pidstat
→ Conclusion
```

正式结果至少记录：

```text
commit
build type
CPU
RAM
filesystem
dataset size
worker count
client count
repetitions
throughput
P50
P95
P99
CPU usage
RSS
Disk IO
```

禁止：

- Debug 构建做正式性能结论；
- 单次运行直接声称 P99；
- 只说“理论上更快”；
- 把单机 VM 数据描述成真实多机集群性能。

报告必须注明：

```text
Ubuntu VM
单机多进程/多容器
多个 Worker 共用同一物理 NVMe
Page Cache 状态
```

---

## 14. Git 工程基线

项目正式开发前先形成稳定工程基线。

第一个稳定提交建议：

```text
chore: bootstrap project baseline
```

至少包含：

```text
.gitignore
AGENTS.md
README.md
.clang-format
CMakeLists.txt
CMakePresets.json
vcpkg.json
include/
src/
tests/
Status / StatusOr
logging
GoogleTest smoke test
```

以下全部通过：

```bash
cmake --preset debug
cmake --build --preset debug
ctest --preset debug --output-on-failure
```

之后建立首个 tag：

```text
v0.0.0-bootstrap
```

该 tag 作为后续所有版本的工程基线。

---

## 15. Git 操作规则

Agent 默认可以：

```text
git status
git diff
git log
建议 commit message
```

Agent 未经用户明确要求不得：

```text
git commit
git tag
git push --force
git reset --hard
git rebase
删除用户分支
```

推荐 Commit 前缀：

```text
feat:
fix:
perf:
refactor:
test:
docs:
build:
chore:
```

例如：

```text
feat: add worker registration and heartbeat
perf: add read plan cache
fix: reject reads from non-ready replicas
```

不要使用：

```text
update
modify
final
final2
fix bug
```

---

## 16. 最高架构约束

### 16.1 大对象不得经过业务 API Server

错误：

```text
Client → Nginx → Artifact API → 8GB Object → Storage
```

正确：

```text
Client → Artifact API 获取版本/凭据
Client → Storage SDK → Worker / Backend
```

### 16.2 大对象不得经过 Storage Control Plane

错误：

```text
Client → Control Plane → Worker → Control Plane → Client
```

正确：

```text
Client → Control Plane → ReadPlan
Client → Worker
```

Control Plane 只处理元数据和调度。

### 16.3 业务管理平面与存储控制平面分离

业务管理平面回答：

```text
用户要哪个 Artifact / Version？
```

存储控制平面回答：

```text
这个 Object / Chunk 在哪里？
```

---

## 17. Object、Replica、Worker 状态

第一阶段 Object 采用：

```text
Immutable Object
```

更新产生新版本，不原地修改已有版本。

Object 状态至少：

```text
ALLOCATING
WRITING
READY
FAILED
DELETING
```

只有 `READY` Object 可以被普通 Get 看见。

Replica 状态至少：

```text
ALLOCATING
WRITING
READY
FAILED
DELETING
```

只有 `READY` Replica 可以进入新的 ReadPlan。

Worker 状态至少：

```text
HEALTHY
SUSPECT
DEAD
```

不得因为单次请求失败立即永久判死节点。

---

## 18. ReadPlan

ReadPlan 是核心抽象之一，应至少描述：

```text
Object
→ Chunks
→ Candidate Replica Locations
→ Backend Fallback
```

Client 根据 ReadPlan 直接访问 Data Plane。

Control Plane 不转发大对象数据。

---

## 19. Placement

Placement Policy 与传输机制分离。

允许按阶段演进：

```text
Hash
→ Consistent Hash
→ Rendezvous Hash
→ Capacity-aware Rendezvous
```

不要把 placement 算法写死在 Worker 内。

---

## 20. Cache 层级

最终目标：

```text
L1 RAM
 ↓
L2 NVMe
 ↓
Backend
```

缓存层必须有稳定接口，调用方不得直接依赖某个 `unordered_map` 或具体文件路径。

---

## 21. MySQL / Redis / Kafka 边界

### MySQL

主要属于业务管理平面：

```text
Project
Artifact
Version
Alias
Tag
UploadSession
PrefetchJob
Audit
Outbox
```

不要默认把整个 Storage Control Plane 简化成 MySQL CRUD。

### Redis

只用于外围高速状态：

```text
Session
Rate Limit
Idempotency Key
Hot Business Metadata
Short-lived Job State
```

禁止 Redis 替代自研 Cache Worker。

### Kafka

只传：

```text
事件
任务
状态变化
```

例如：

```text
ArtifactUploadCompleted
PrefetchRequested
WorkerDead
ReplicaRepaired
```

禁止通过 Kafka 搬运 GB 级对象。

---

## 22. Transactional Outbox

业务数据库状态与 Kafka 事件需要一致时，优先采用 Outbox：

```text
BEGIN
  UPDATE business state
  INSERT outbox_event
COMMIT
```

由独立 Relay 投递 Kafka。

必须考虑：

```text
重复投递
Relay Crash
Consumer Restart
Idempotent Consumer
```

---

## 23. 异步任务

耗时任务不得长期阻塞 API 请求，例如：

```text
Prefetch
GC
后台校验
Repair
```

推荐：

```text
提交任务
→ 返回 task_id
→ 后台执行
→ 查询状态
```

业务层采用：

```text
模块化单体 + 独立异步 Worker
```

第一阶段不要为了“微服务”拆出大量空壳服务。

---

## 24. 网络与协议

控制面跨进程消息优先：

```text
protobuf + gRPC
```

Data Plane 按阶段演进：

```text
V1: 简单可靠传输，先闭环
V2+: 独立 TCP Data Plane
后续: sendfile / io_uring / zero-copy 实验
```

不要在 V0 自研完整 RPC 框架。

---

## 25. Checksum

大对象完整性统一通过明确 Checksum 抽象。

第一阶段建议：

```text
BLAKE3
```

Checksum 逻辑不得散落在业务层。

---

## 26. 并发与背压

所有高并发队列最终必须有界。

必须逐步引入：

```text
bounded queue
inflight limit
timeout
retry budget
cancellation
backpressure
```

关键路径禁止：

- 无限创建线程；
- 无界任务队列；
- 无限 retry；
- 高频同步日志；
- 不必要的大 Buffer 复制；
- 每请求重复大块 malloc/free。

共享状态必须明确：

```text
谁拥有
谁写
谁读
锁粒度
```

不要用全局大锁掩盖设计问题。

---

## 27. 时间与大小类型

时间统一使用：

```cpp
std::chrono
```

不要裸传 `int timeout`。

文件/Object/Chunk 大小优先：

```cpp
std::uint64_t
```

不要用 `int` 表示大对象大小。

---

## 28. Docker 与 Native Benchmark

Docker 用于：

```text
环境复现
一键部署
多节点模拟
故障注入
集成测试
```

最终完整部署应支持：

```bash
docker compose up -d
```

但底层性能关键路径必须保留 Native Release 对照测试：

```text
Docker → 工程部署验证
Native Release → perf/strace/io_uring 等性能分析
```

---

## 29. 可观测性

最终使用：

```text
Prometheus + Grafana
```

至少覆盖：

```text
request latency
throughput
RAM/NVMe/remote/backend hit
backend bytes
worker capacity
worker health
replica repair
queue depth
readplan cache hit
outbox pending
kafka lag
```

日志与 Metrics 不混用。

---

## 30. 当前版本开发顺序

严格按阶段推进：

```text
V0  工程骨架
V1  单 Worker Put/Get
V2  多 Worker + Control/Data Plane 分离
V3  RAM/NVMe Tiered Cache
V4  Placement + Replication
V5  Failure + Repair
V6  Prefetch + ReadPlan Cache + Hedged Read
V7  Linux IO 性能优化
V8  业务管理平面
V9  Kafka + Outbox + Async Job
V10 Docker + Observability
V11 Final Benchmark
```

V0~V2 默认禁止为了“以后可能用”提前引入：

```text
MySQL
Redis
Kafka
Kubernetes
etcd
Raft
RDMA
CUDA
GPUDirect
FUSE
Service Mesh
```

第一阶段只优先完成：

```text
Client
→ Control Plane
→ Worker
→ File

Put
Get
Checksum
ReadPlan
Multi-worker
```

---

## 31. 阶段完成定义

每个阶段至少满足：

```text
代码完成
+ 编译通过
+ Unit Test
+ 必要的 Integration Test
+ 必要的 Benchmark
+ 文档更新
+ git diff 可审查
```

然后才进入下一阶段。

---

## 32. Agent 修改代码前必须做什么

修改现有模块前必须先：

1. 阅读当前模块；
2. 阅读相关接口；
3. 阅读相关测试；
4. 确认当前版本阶段；
5. 判断是否破坏架构边界；
6. 选择最小必要修改。

禁止直接创建一套平行实现来绕过现有代码。

---

## 33. Agent 修改代码后必须做什么

至少检查：

```text
format
build
unit test
integration test（如相关）
sanitizer（重要内存/并发改动）
```

性能修改还必须有：

```text
baseline
修改后 benchmark
```

不能因为“编译通过”就宣称完成。

---

## 34. 不允许隐式改变的语义

以下变化必须明确指出：

```text
协议
持久化格式
状态机
错误码
线程模型
默认 timeout
缓存一致性
对象可见性
```

不得以“重构”为名偷偷改变行为。

---

## 35. 文档

至少维护：

```text
README.md
AGENTS.md

docs/
├── architecture/
├── design/
└── benchmark/
```

README 面向使用者、开发者和面试官。

AGENTS.md 只负责工程与 Agent 规则，不代替 README 和设计文档。

---

## 36. 安全

禁止提交：

```text
password
token
private key
真实云账号
真实数据库凭据
```

使用：

```text
.env.example
配置模板
环境变量
```

---

## 37. 最终原则

所有开发决策优先级：

```text
正确性
>
架构边界
>
可维护性
>
可测试性
>
性能
>
功能数量
```

性能关键路径经过 Benchmark 证明后，可以为了性能做适度复杂化，但必须保留清晰接口、测试、性能证据和文档。

---

## 38. Agent 最终检查清单

```text
[ ] 是否遵守当前版本范围？
[ ] 是否破坏 Control Plane / Data Plane 分离？
[ ] 是否引入不必要依赖？
[ ] 是否遵守 C++20 / RAII / Status 规则？
[ ] 是否正确处理错误和边界条件？
[ ] 是否新增或更新测试？
[ ] 是否能通过 CMakePresets 构建？
[ ] 是否需要 Sanitizer？
[ ] 若为性能修改，是否已有 Baseline？
[ ] 是否产生与任务无关的大 diff？
[ ] 是否擅自 commit / tag？
[ ] 是否需要更新设计或 Benchmark 文档？
```

如果关键项无法确认，必须明确指出，不得假设完成。
