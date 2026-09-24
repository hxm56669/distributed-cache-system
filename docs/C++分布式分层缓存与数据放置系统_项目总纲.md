# C++ 分布式分层缓存与数据放置系统 —— 项目总纲

> 项目定位：面向 AI 模型、数据集、Checkpoint 等“读多写少、大对象、高并发”场景，构建一个 **可用、可部署、可验证** 的 C++ 分布式分层缓存与数据放置系统。  
> 核心目标：不追求工业系统的集群规模，但尽量复现工业系统最关键的机制，包括控制面/数据面分离、ReadPlan、分层缓存、数据放置、复制与修复、预取、尾延迟优化、异步任务、可靠事件、容器化部署与完整性能验证。

---

## 1. 项目最终定位

项目不是：

- 一个简单的 `unordered_map + LRU` 缓存；
- 一个只有 Master/Worker 的课程作业；
- 一个给普通分布式缓存硬贴 “AI Storage” 标签的项目；
- 一个为了堆 Redis、Kafka、MySQL、Docker 而拼接出来的中间件集合；
- 一个直接复刻 Mooncake、Alluxio 的超大工程。

项目最终定位为：

> **面向 AI 大对象访问场景的 C++ 分布式分层缓存与数据放置系统，并在其上构建一个模型/数据集制品管理服务。**

系统划分为三个平面：

1. **业务管理平面**：用户、项目、制品、版本、Alias/Tag、上传会话、预取任务、权限、审计、可靠事件。
2. **存储控制平面**：Metadata、ReadPlan、Cache Directory、Placement、Membership、Failure Detection、Replication、Repair、Recovery。
3. **存储数据平面**：真正的大对象读取、传输、缓存、校验以及 RAM/NVMe 分层存储。

---

# 2. 为什么选择这个项目

这个项目主要解决以下问题：

```text
AI 模型 / 数据集 / Checkpoint
            │
            │ 大对象、读多写少、热点明显
            ▼
      远端对象存储
            │
            │ 首次读取慢、重复读取浪费带宽
            ▼
     分布式缓存层
            │
      RAM / NVMe Cache
            │
            ▼
       计算节点 / Client
```

主要工程目标：

- 减少重复访问后端对象存储；
- 提高热点大对象重复读取速度；
- 降低后端流量；
- 降低 P95/P99 尾延迟；
- 支持多个缓存节点动态加入、退出和故障；
- 支持 RAM/NVMe 分层缓存；
- 控制面不进入真正的大对象数据路径；
- 可以通过 Client SDK、CLI 和业务 API 真正使用；
- 可以通过 Docker Compose 一键部署；
- 可以通过 Benchmark、`perf`、`strace`、`iostat` 等工具证明优化效果。

---

# 3. 参考架构

项目不是凭空设计，而是对多个真实系统进行有限范围重实现。

## 3.1 Meta Owl

主要参考：

```text
Centralized Control Plane
+
Decentralized Data Plane
+
Mechanism / Policy Separation
```

对应到本项目：

- Control Plane 决定数据在哪里、应该从哪里读、应该放到哪里；
- Cache Worker 只负责缓存、读取和传输；
- 真正数据不经过 Control Plane。

参考：

- Meta Engineering — Owl: Distributing content at Meta scale  
  https://engineering.fb.com/2022/07/14/data-infrastructure/owl-distributing-content-at-meta-scale/

---

## 3.2 Alluxio

主要参考：

```text
Master / Worker
Block Location
Local Cache Hit
Remote Cache Hit
Cache Miss
Tiered Storage
Asynchronous Cache
```

对应到本项目：

```text
Local RAM/NVMe Hit
        ↓ miss
Remote Worker Hit
        ↓ miss
Backend Object Storage
```

参考：

- Alluxio Architecture  
  https://docs.alluxio.io/os/user/stable/en/overview/Architecture.html

---

## 3.3 Mooncake Store

主要参考：

```text
Master Service
Object Placement
Node Membership
RAM / SSD
Replication
Eviction
Snapshot / Restore
Client-to-Client Data Transfer
```

最重要的一点：

> Master 只负责控制和元数据，不承担真实大对象数据流量。

对应到本项目：

- Control Plane；
- Cache Worker；
- PutStart / Commit；
- Replica；
- RAM/NVMe；
- 节点心跳；
- Metadata Snapshot / Recovery。

参考：

- Mooncake Store Design  
  https://github.com/kvcache-ai/Mooncake/blob/main/docs/source/design/store/mooncake-store.md
- Mooncake Architecture  
  https://github.com/kvcache-ai/Mooncake/blob/main/docs/source/design/architecture.md

---

## 3.4 Meta 2026 AI Storage

主要参考：

```text
Unified Metadata
ReadPlan
ReadPlan Cache
Distributed Data Cache
Prefetch
Hedged Read
Dynamic Concurrency
```

其中最值得本项目复现的是：

```text
Client
  ↓ metadata lookup
Control Plane
  ↓
ReadPlan
  ↓
Client 直接访问 Storage / Cache Worker
```

参考：

- Meta’s AI Storage Blueprint at Scale  
  https://engineering.fb.com/2026/07/01/data-infrastructure/metas-ai-storage-blueprint-at-scale/

---

## 3.5 LMCache

主要参考：

```text
GPU
 ↓
CPU DRAM
 ↓
Local SSD / NVMe
 ↓
Remote Backend
```

本项目暂时不实现 GPU KV Cache，而只保留：

```text
RAM
 ↓
NVMe
 ↓
Backend
```

同时参考：

- 异步 Offload；
- Prefetch；
- Eviction；
- 多层缓存；
- Cache Management API。

参考：

- LMCache Architecture  
  https://docs.lmcache.ai/developer_guide/architecture.html

---

# 4. 业务管理平面的参考架构

业务层不是随意堆中间件，而是参考成熟的制品管理系统。

## 4.1 MLflow Model Registry

主要借鉴：

```text
Artifact / Model
      │
      ├── Version 1
      ├── Version 2
      └── Version 3
             │
             ├── Alias
             └── Tags
```

业务层需要支持：

- Version；
- Alias；
- Tag；
- Metadata；
- Lifecycle。

参考：

- MLflow Model Registry  
  https://mlflow.org/docs/latest/model-registry/

---

## 4.2 Harbor

主要借鉴：

- Project；
- Artifact；
- Core Service；
- Database；
- Redis；
- Job Service；
- Registry；
- Proxy / Nginx；
- Metrics；
- 后台任务。

本项目不复制 Harbor，而只参考其“业务 Core + 异步 Job + Registry/Data”分工。

参考：

- Harbor Documentation  
  https://goharbor.io/docs/

---

## 4.3 Transactional Outbox

业务数据库更新与 Kafka 事件发送存在典型双写问题：

```text
MySQL COMMIT 成功
        ↓
Kafka Produce 失败
        ↓
数据库状态已经变化
但下游完全不知道
```

因此采用：

```text
BEGIN

UPDATE / INSERT business tables
INSERT outbox_events

COMMIT
```

再由 Outbox Relay 异步投递 Kafka。

参考：

- AWS Transactional Outbox Pattern  
  https://docs.aws.amazon.com/en_en/prescriptive-guidance/latest/cloud-design-patterns/transactional-outbox.html

---

# 5. 最终总体架构

```text
========================================================
                    用户 / CLI / SDK
========================================================
                           │
                           ▼
                      Nginx / TLS
                           │
                           ▼
                 C++ Artifact API Server
                 ┌──────────────────────┐
                 │ Project              │
                 │ Artifact             │
                 │ Version              │
                 │ Alias / Tag          │
                 │ UploadSession        │
                 │ PrefetchJob          │
                 │ Auth / Idempotency   │
                 │ Rate Limit           │
                 └───────┬───────┬──────┘
                         │       │
                   ┌─────▼───┐ ┌─▼───────┐
                   │ MySQL   │ │ Redis   │
                   │ 真相源  │ │ Cache   │
                   └────┬────┘ └─────────┘
                        │
                  outbox_events
                        │
                        ▼
                   Outbox Relay
                        │
                        ▼
                       Kafka
                  ┌─────┼───────┐
                  ▼     ▼       ▼
              Prefetch  GC    Audit
               Worker  Worker  Consumer
                  │
                  │ Storage Client SDK
                  ▼

========================================================
                    存储控制平面
========================================================

                   Control Plane
           ┌─────────────────────────┐
           │ Metadata Directory      │
           │ Cache Directory         │
           │ ReadPlan                │
           │ Placement               │
           │ Membership              │
           │ Failure Detector        │
           │ Replication             │
           │ Repair Scheduler        │
           │ Snapshot / Recovery     │
           └────────────┬────────────┘
                        │
                   返回 ReadPlan
                        │

========================================================
                     存储数据平面
========================================================

             ┌──────────┼──────────┐
             ▼          ▼          ▼
          Worker A   Worker B   Worker C
          RAM Cache  RAM Cache  RAM Cache
             ↓          ↓          ↓
          NVMe Cache NVMe Cache NVMe Cache
             │          │          │
             └────── Data Plane ───┘
                        │
                        ▼
                  Backend / MinIO
```

---

# 6. 三个平面的职责边界

## 6.1 业务管理平面

负责：

```text
谁在使用数据
数据属于哪个 Project
Artifact 有哪些 Version
production Alias 指向哪个 Version
上传任务是否完成
预取任务是否完成
用户是否有权限
业务事件是否可靠发出
```

不负责：

```text
Chunk 在哪个 Worker
真正大文件数据传输
缓存淘汰
Replica Repair
NVMe IO
```

---

## 6.2 存储控制平面

负责：

```text
Object → Chunk
Chunk → Replica
Replica → Worker

Worker Capacity
Worker Health

Placement
ReadPlan
Cache Directory
Replication
Repair
Metadata Recovery
```

控制面尽量只处理小元数据。

---

## 6.3 数据平面

负责：

```text
真正的数据读取
真正的数据写入
Worker ↔ Client
Worker ↔ Worker
Backend → Worker
RAM ↔ NVMe
Checksum
```

原则：

> **大对象数据尽量不经过业务服务器，也不经过存储 Control Plane。**

---

# 7. 业务数据模型

推荐核心实体：

```text
User
 │
Project
 │
Artifact
 │
ArtifactVersion
 ├── Alias
 └── Tags

UploadSession
PrefetchJob
AuditLog
OutboxEvent
```

核心表建议：

```text
users
projects
project_members

artifacts
artifact_versions
artifact_aliases
artifact_tags

upload_sessions
prefetch_jobs

audit_logs
outbox_events
```

`artifact_versions` 示例字段：

```text
id
artifact_id
version
object_key
size
checksum
status
created_at
created_by
```

业务数据库只存：

```text
object_key
size
checksum
状态
版本关系
```

不存大文件本体。

---

# 8. 存储数据模型

建议：

```text
Object
  │
  ├── Chunk 0
  │      ├── Replica A
  │      └── Replica B
  │
  ├── Chunk 1
  │      ├── Replica A
  │      └── Replica B
  │
  └── Chunk N
```

主要对象：

```text
ObjectMetadata
ChunkMetadata
ReplicaMetadata
WorkerMetadata
ReadPlan
UploadPlan
```

Replica 状态建议：

```text
ALLOCATING
WRITING
READY
FAILED
DELETING
```

---

# 9. 写路径

目标：不可变对象 + 完整对象可见性。

```text
Client
  │
  │ PutStart(key, size)
  ▼
Control Plane
  │
  │ Placement
  ▼
UploadPlan

Chunk0 → Worker A / B
Chunk1 → Worker B / C
Chunk2 → Worker A / C

Client
  │
  │ 真实数据直传
  ▼
Workers

Checksum
  ↓
Worker ACK
  ↓
PutCommit
  ↓
Object = READY
```

语义：

```text
Commit 前
→ Get 不应看到半个对象

Commit 后
→ Get 得到完整对象
```

第一阶段对象采用 immutable model：

```text
同一个 key/version 不原地修改
新版本产生新 object
```

这样可以明显降低一致性复杂度。

---

# 10. 读路径

```text
Client Get(object_key)
        │
        ▼
Control Plane
        │
        ▼
ReadPlan
{
    Chunk0 → local Worker A
    Chunk1 → remote Worker B
    Chunk2 → Backend
}
        │
        ▼
Client 并行读取
        │
        ▼
Checksum / Assemble
```

读取优先级：

```text
Local RAM Hit
      ↓ miss
Local NVMe Hit
      ↓ miss
Remote Worker Hit
      ↓ miss
Backend
```

读取 Backend 后可进行异步 cache fill。

---

# 11. 业务上传流程

例如上传一个 8GB 模型：

```text
POST /api/v1/projects/demo/artifacts/qwen/versions
```

Artifact API：

```text
MySQL:
Version = 7
Status = UPLOADING
```

然后申请底层 UploadPlan。

返回：

```text
version_id
upload_session
object_key
upload_plan
```

真正的 8GB 文件：

```text
Client
  ↓
Storage SDK
  ↓
Cache Worker / Backend
```

不会经过：

```text
Nginx
Artifact API Server
MySQL
Kafka
```

完成后：

```text
Storage Commit
      ↓
Artifact API
      ↓
MySQL:
UPLOADING → READY

+
OutboxEvent:
ArtifactUploadCompleted
```

---

# 12. 业务下载流程

用户：

```text
cachectl pull qwen@production
```

第一步：

```text
Artifact API
      ↓
production → Version 7
      ↓
返回 object_key / checksum / size
```

第二步：

```text
Storage SDK
      ↓
Control Plane
      ↓
ReadPlan
      ↓
Worker A/B/C
      ↓
真正下载
```

因此：

```text
业务管理平面：
你要的是哪个版本？

存储控制平面：
这个版本的数据在哪里？

数据平面：
真正把数据传给你。
```

---

# 13. Prefetch

用户：

```text
POST /prefetch
```

API 不等待大文件真正预热完成，而是：

```text
HTTP
 ↓
202 Accepted
 ↓
task_id
```

后台：

```text
Kafka
  ↓
Prefetch Worker
  ↓
Storage SDK
  ↓
Backend
  ↓
NVMe
  ↓
RAM
```

用户查询：

```text
GET /tasks/{task_id}
```

得到：

```text
PENDING
RUNNING
SUCCEEDED
FAILED
```

支持：

- 显式 Prefetch；
- 顺序访问时预取 N+1/N+2；
- 热点对象预热。

---

# 14. 数据放置策略

不要只做：

```text
hash(key) % worker_count
```

演进路线：

## V1

```text
普通 Hash
```

## V2

```text
Consistent Hash
```

## V3

```text
Rendezvous Hashing
```

## V4

加入容量与负载：

```text
PlacementScore =
    HashScore
    × CapacityWeight
    × LoadWeight
    × HealthWeight
```

考虑：

```text
Worker Capacity
Worker Used Space
Worker Load
Replica Count
Node Health
```

Benchmark：

```text
节点加入 / 退出后的数据迁移比例
负载标准差
最大节点利用率
缓存命中率
```

---

# 15. Cache Policy

基础版：

```text
LRU
```

进阶版：

```text
SLRU
+
Admission Policy
```

进一步研究：

```text
TinyLFU 类准入机制
```

目标解决：

```text
一次性 Sequential Scan
        ↓
把真正热点数据全部挤出缓存
        ↓
Cache Pollution
```

Workload：

```text
Uniform Random
Zipf
80/20 Hotspot
Sequential Scan
Mixed
```

对比：

```text
Hit Ratio
Backend Traffic
P95/P99
NVMe Write Bytes
```

---

# 16. RAM / NVMe 分层缓存

建议：

```text
L1：RAM
    低容量 / 低延迟

L2：NVMe
    大容量 / 较低延迟

L3：Backend / MinIO
    持久后端
```

状态迁移：

```text
RAM Hit
→ 直接读

RAM Miss + NVMe Hit
→ NVMe read
→ 根据策略晋升 RAM

NVMe Miss
→ Backend read
→ NVMe cache fill
→ 可选 RAM cache fill
```

需要解决：

- 多层容量管理；
- Promotion；
- Demotion；
- Eviction；
- Admission；
- 正在使用对象禁止淘汰；
- 并发 Get 同一对象避免重复回源。

---

# 17. 节点管理与故障恢复

Worker 定期：

```text
Heartbeat
```

Control Plane 维护：

```text
HEALTHY
SUSPECT
DEAD
```

示例：

```text
Worker A 超过阈值无心跳
        ↓
A = DEAD
        ↓
Chunk X 原本：
A + B
        ↓
只剩 B
        ↓
Repair Scheduler
        ↓
B → C
        ↓
重新恢复目标副本数
```

客户端读取：

```text
请求 A
 ↓ failed
请求 B
 ↓ success
上报 / 记录失败
```

必须支持：

- Worker Crash；
- Worker Restart；
- Rejoin；
- Replica Repair；
- Read Fallback。

---

# 18. Hedged Read

目标：解决尾延迟。

```text
Chunk X:
A、B 都有 Replica

先请求 A
   ↓
超过 hedge threshold
   ↓
并行请求 B
```

例如：

```text
A ───────── 120ms
B ── 15ms ✓
```

采用先完成的结果。

Benchmark：

```text
人为注入：
50ms
100ms
200ms
```

比较：

```text
普通读取
vs
Hedged Read

P50
P95
P99
额外网络流量
```

---

# 19. ReadPlan Cache

基础：

```text
Client
→ 每次请求 Control Plane
→ Metadata Lookup
→ ReadPlan
```

优化：

```text
ReadPlan Cache

object_key
→
chunk locations
```

需要处理：

- TTL；
- Worker 失效导致 ReadPlan 过期；
- 版本号 / generation；
- 读取失败后的重新查询。

Benchmark：

```text
无 ReadPlan Cache
vs
有 ReadPlan Cache

Metadata QPS
CPU
P50
P95
P99
```

---

# 20. 并发与背压

不要无限创建线程或任务。

建议：

```text
Request
  ↓
Bounded Queue
  ↓
Worker Pool
  ↓
IO Engine
```

需要：

- bounded queue；
- per-worker inflight limit；
- timeout；
- cancellation；
- retry budget；
- backpressure；
- connection reuse。

可进一步研究动态并发：

```text
Concurrency
4 → 8 → 16 → 32 → 64
```

根据：

```text
RTT
P99
Queue Depth
Timeout Rate
Throughput
```

调整。

---

# 21. Linux 数据面性能优化

先建立简单稳定基线，再逐步优化。

## Baseline

```text
pread
write / send
普通 buffer
```

## 后续实验

```text
read/write
vs
sendfile
vs
io_uring
```

以及：

- Buffer Pool；
- 减少 malloc/free；
- Chunk 并行读取；
- Batch Completion；
- Connection Pool；
- Zero-copy 路径；
- 大对象 striping；
- Checksum pipeline；
- CPU / IO 重叠。

验证工具：

```text
perf
strace
iostat
pidstat
```

原则：

> 每一项优化都必须有“优化前基线 → 改造 → 数据 → 瓶颈解释”。

---

# 22. MySQL 的职责

MySQL 是业务真相源。

负责：

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

可以体现普通 C++ 后端能力：

- Schema 设计；
- 索引；
- 唯一约束；
- 事务；
- 隔离级别；
- 慢 SQL；
- 连接池；
- 乐观并发；
- 分页；
- 幂等。

不要把底层 Chunk Location、缓存目录等全部交给 MySQL，否则会削弱自研 Storage Control Plane 的价值。

---

# 23. Redis 的职责

Redis 只负责外围高速状态：

```text
Session / Token
Rate Limit
Idempotency Key
Hot Business Metadata
Short-lived Upload Session
Short-lived Job Status
```

不使用 Redis 替代自研 Cache Worker。

---

# 24. Kafka 的职责

Kafka 不是大文件传输通道。

只传领域事件：

```text
ArtifactVersionCreated
ArtifactUploadCompleted
ArtifactDeleted
PrefetchRequested
PrefetchCompleted
CacheWarmupFailed
WorkerDead
ReplicaRepaired
```

可以体现：

- Producer；
- Consumer Group；
- Retry；
- Duplicate；
- Idempotent Consumer；
- Consumer Lag；
- Message Ordering；
- Backlog。

---

# 25. Outbox

示例：

```text
BEGIN;

UPDATE artifact_versions
SET status = 'READY';

INSERT INTO outbox_events(...);

COMMIT;
```

Outbox Relay：

```text
扫描 PENDING
   ↓
Kafka Produce
   ↓
成功
   ↓
标记 SENT
```

必须考虑：

```text
Relay Crash
重复发送
至少一次投递
消费端幂等
```

这是业务层值得重点讲的可靠性机制之一。

---

# 26. C++ Artifact API Server

不拆成十几个微服务。

采用：

> **模块化单体 + 独立异步 Worker**

推荐目录：

```text
src/business/

api/
    artifact_controller
    project_controller
    task_controller

service/
    artifact_service
    project_service
    upload_service
    prefetch_service

repository/
    artifact_repository
    project_repository
    job_repository

domain/
    project
    artifact
    artifact_version
    upload_session
    job

infra/
    mysql
    redis
    kafka
    storage_client
```

单体内部结构必须清晰，但不为了“微服务”而拆服务。

---

# 27. REST API 初步边界

建议至少支持：

```text
POST   /api/v1/projects
GET    /api/v1/projects/{id}

POST   /api/v1/projects/{project}/artifacts
GET    /api/v1/projects/{project}/artifacts/{name}

POST   /api/v1/projects/{project}/artifacts/{name}/versions
GET    /api/v1/projects/{project}/artifacts/{name}/versions/{version}

PUT    /api/v1/projects/{project}/artifacts/{name}/aliases/{alias}

POST   /api/v1/uploads
POST   /api/v1/uploads/{id}/commit

POST   /api/v1/prefetch
GET    /api/v1/tasks/{id}

GET    /api/v1/nodes
GET    /metrics
```

---

# 28. Client CLI

最终项目必须真正可以使用。

例如：

```bash
cachectl login

cachectl project create demo

cachectl push \
  demo/qwen:v1 \
  ./qwen-model.bin

cachectl alias set \
  demo/qwen \
  production \
  v1

cachectl pull \
  demo/qwen@production \
  -o ./qwen-model.bin

cachectl prefetch \
  demo/qwen@production

cachectl task status 123

cachectl nodes

cachectl cluster-status
```

CLI 是项目“能用”的重要证明。

---

# 29. C++ Client SDK

除了 CLI，还应提供：

```cpp
ArtifactClient client(...);

auto artifact =
    client.Resolve("demo/qwen@production");

DistributedCache cache(...);

cache.Prefetch(artifact.object_key);

auto result =
    cache.Get(artifact.object_key);
```

这样可以让其他 C++ 程序真正接入。

---

# 30. Docker Compose 部署

最终要求：

```bash
git clone ...
docker compose up -d
```

能够启动完整环境：

```text
nginx

artifact-api-1
artifact-api-2

job-worker
outbox-relay

mysql
redis
kafka

control-plane

cache-worker-1
cache-worker-2
cache-worker-3

minio

prometheus
grafana
```

Docker 模式主要证明：

- 一键部署；
- 服务编排；
- 环境隔离；
- 多节点模拟；
- 故障实验；
- 持久化 Volume；
- 监控。

---

# 31. Docker 与 Benchmark 必须分开

## 部署验证

```text
Ubuntu VM
  ↓
Docker Compose
```

验证：

```text
部署
服务发现
故障恢复
持久化
监控
完整业务链路
```

## 性能测试

```text
Ubuntu VM
  ↓
Native Release Binary
```

用于：

```text
perf
strace
iostat
pidstat
io_uring
native filesystem benchmark
```

不要把 Docker 性能直接当作底层极限性能结论。

---

# 32. 当前硬件利用方案

宿主机：

```text
CPU：
Intel Core i7-13700F

RAM：
32GB DDR5 4800

GPU：
RTX 5060 8GB

NVMe：
约 300GB 可用
```

当前项目主体并不依赖 GPU。

推荐 Ubuntu VM：

```text
CPU：12 vCPU
RAM：20~24GB
Disk：约 220GB 动态虚拟磁盘
Network：桥接
Filesystem：ext4
```

Windows 保留：

```text
8~12GB RAM
其余 CPU 资源
约 80GB 以上磁盘余量
```

---

# 33. Ubuntu VM 内存建议

假设 Ubuntu 分配 24GB：

```text
Ubuntu OS             2~3GB

Worker1 RAM Cache     2GB
Worker2 RAM Cache     2GB
Worker3 RAM Cache     2GB
Worker4 RAM Cache     2GB

MySQL                 ~1GB
Kafka                 1~2GB
MinIO                 ~1GB

Control Plane         0.5~1GB

API / Redis / Worker  1~2GB

Prometheus / Grafana  ~1GB

剩余：
Linux Page Cache
Build
Benchmark
```

根据阶段动态调整，不需要一次全部跑满。

---

# 34. NVMe 规划

建议 Ubuntu VM 动态盘约 220GB。

测试目录示例：

```text
/data/

worker1-cache/     25GB
worker2-cache/     25GB
worker3-cache/     25GB
worker4-cache/     25GB

minio/             60~80GB

mysql/
kafka/
metrics/

benchmark/
```

注意：

> 多个 Worker 最终仍然共用同一块物理 NVMe。

因此能够验证：

- 软件架构；
- IO 路径；
- Cache Policy；
- Placement；
- 故障；
- 相对性能优化。

但不能声称等价于真正多机、多 NVMe 集群的绝对性能。

---

# 35. 网络环境

推荐 Ubuntu VM 使用桥接网络：

```text
Windows Host
     │
     ├── Ubuntu VM
     ├── Laptop / Other Client
     └── Phone / LAN Client
```

这样 Ubuntu VM 可以作为真正的局域网服务器被访问。

开发方式：

```text
Windows
   ↓
VSCode Remote SSH
   ↓
Ubuntu VM
```

---

# 36. Benchmark 体系

最终 Benchmark 不只测吞吐。

## 36.1 Cache Path

```text
Backend Direct
Local RAM Hit
Local NVMe Hit
Remote Cache Hit
Cache Miss
```

测：

```text
Throughput
P50
P95
P99
Backend Traffic
CPU
Disk IO
```

---

## 36.2 Client 并发

```text
1
2
4
8
16
32
64
```

观察：

```text
Throughput Scaling
P99
Queue Depth
Timeout
CPU Saturation
```

---

## 36.3 Worker 扩展性

```text
1 Worker
2 Workers
3 Workers
4 Workers
```

观察：

```text
Throughput
Load Balance
Cache Hit
Placement Distribution
```

---

## 36.4 Cache Policy

```text
LRU
SLRU
Admission + SLRU
```

Workload：

```text
Zipf
Random
Sequential
80/20 Hotspot
```

---

## 36.5 Placement

```text
Hash
Consistent Hash
Rendezvous Hash
Capacity-aware Rendezvous
```

测：

```text
Migration Ratio
Load StdDev
Max Worker Load
Capacity Utilization
```

---

## 36.6 Prefetch

```text
No Prefetch
Sequential Prefetch
Explicit Prefetch
```

测：

```text
First-byte Latency
Full-object Latency
Backend Traffic
Cache Hit
```

---

## 36.7 Failure

```text
Kill Worker
Network Delay
Worker Restart
Control Plane Restart
```

测：

```text
Request Failure Rate
Recovery Time
Replica Repair Time
P99
```

---

## 36.8 Hedged Read

注入：

```text
50ms
100ms
200ms
```

测：

```text
P95/P99
Additional Traffic
Success Rate
```

---

## 36.9 ReadPlan Cache

```text
Cache OFF
Cache ON
```

测：

```text
Metadata QPS
CPU
P50
P95
P99
```

---

# 37. 可观测性

Prometheus 指标至少包括：

## Business

```text
http_requests_total
http_request_duration
mysql_pool_in_use
redis_latency
kafka_producer_errors
kafka_consumer_lag
outbox_pending
job_queue_depth
```

## Storage

```text
storage_get_total
storage_put_total

read_latency
write_latency

ram_cache_hit
nvme_cache_hit
remote_cache_hit
backend_miss

backend_bytes_read

worker_capacity
worker_used_bytes

replica_repair_total
worker_health

readplan_cache_hit

hedged_read_total
hedged_read_winner
```

Grafana 形成 Dashboard。

---

# 38. 推荐开发阶段

不要一开始同时写 MySQL、Kafka、缓存、io_uring。

---

## V0：工程骨架

目标：

- CMake；
- 单元测试；
- Status/StatusOr；
- 配置；
- 日志；
- protobuf/RPC；
- 基本目录结构。

验收：

```text
Control Plane
Worker
Client
三个进程能够启动、通信
```

---

## V1：单 Worker 最小存储闭环

实现：

```text
Put
Get
Delete
Head
Checksum
Backend
```

暂不做：

```text
Redis
Kafka
Replication
高级 Cache
```

验收：

```text
真实 1GB/5GB 文件 Put/Get 正确
checksum 一致
```

---

## V2：多 Worker + Control/Data Plane 分离

实现：

```text
Worker Registration
Heartbeat
Metadata
Chunk
ReadPlan
Client Direct Read
```

验收：

```text
1 Control Plane
3 Worker
Client 不通过 Control Plane 传大数据
```

---

## V3：RAM/NVMe Tiered Cache

实现：

```text
RAM Cache
NVMe Cache
Backend
LRU
Promotion
Eviction
```

验收：

```text
RAM Hit
NVMe Hit
Remote Hit
Backend Miss
```

都有可证明路径。

---

## V4：Placement + Replication

实现：

```text
Rendezvous Hashing
Capacity Weight
Replica = 2
```

验证：

```text
扩容 / 缩容
迁移比例
负载均衡
```

---

## V5：Failure + Repair

实现：

```text
Failure Detector
Read Fallback
Repair Scheduler
Rejoin
```

验收：

```text
kill worker
Client 仍可读取
Replica 自动恢复
```

---

## V6：Prefetch + ReadPlan Cache + Hedged Read

实现三项有明显工业价值的优化：

```text
Prefetch
ReadPlan Cache
Hedged Read
```

每项都要：

```text
Baseline
Optimization
Benchmark
Bottleneck Analysis
```

---

## V7：Linux IO 性能优化

实验：

```text
read/write
sendfile
io_uring

Buffer Pool
Parallel Chunk IO
Connection Reuse
Backpressure
```

不是所有优化都必须最终保留，以数据决定。

---

## V8：业务管理平面

增加：

```text
C++ Artifact API
MySQL
Redis
Version
Alias
Tag
UploadSession
PrefetchJob
```

形成真正用户可用产品入口。

---

## V9：Kafka + Outbox + Async Job

实现：

```text
Kafka
Outbox
Prefetch Worker
GC Worker
Audit Consumer
Idempotent Consumer
```

重点验证：

```text
DB commit 后 relay crash
Kafka duplicate
Consumer restart
```

---

## V10：部署与可观测性

完成：

```text
Dockerfile
docker-compose.yml

Nginx
Prometheus
Grafana
MinIO
```

做到：

```bash
docker compose up -d
```

一键启动完整系统。

---

## V11：Final Benchmark

形成完整测试报告：

```text
功能
性能
扩展性
缓存
Placement
故障
Recovery
Prefetch
Hedged Read
ReadPlan Cache
IO Optimization
资源消耗
```

---

# 39. 最终验收标准

项目只有达到下面状态，才视为“毕业”。

```text
git clone
   ↓
docker compose up -d
   ↓
整个系统启动
```

然后：

### 1. 上传真实大对象

```text
cachectl push model:v1 5GB.bin
```

成功。

### 2. 第一次读取

```text
Backend Miss
```

成功。

### 3. 第二次读取

```text
RAM / NVMe Cache Hit
```

成功，并且后端流量明显下降。

### 4. 多节点

```text
3~4 Cache Worker
```

数据能够分布。

### 5. Worker Failure

```text
kill worker-2
```

Client 仍能够从其他 Replica 获取数据。

### 6. Repair

系统自动：

```text
B → C
```

重新恢复副本数。

### 7. Control Plane Restart

重启后：

```text
Metadata Restore
```

能够继续服务。

### 8. Prefetch

Prefetch 后实际 Get 明显减少 Backend 访问。

### 9. Hedged Read

Slow Worker 注入后，P99 有可量化改善。

### 10. Metrics

Grafana 能看到：

```text
Throughput
P99
Cache Hit
Backend Traffic
Worker Health
Capacity
Kafka Lag
Job Queue
```

### 11. Benchmark

存在完整：

```text
优化前
vs
优化后
```

报告，而不是只给最终数字。

---

# 40. 明确不做的内容

核心版本暂不做：

```text
Raft
多 Master 强一致
跨 Region
完整 POSIX 文件系统
FUSE
完整 S3 协议
Erasure Coding
RDMA
GPUDirect
多 GPU KV Cache
分布式事务框架
Service Mesh
十几个微服务
```

原因：

```text
实现成本巨大
+
当前硬件环境难以可靠验证
+
会稀释项目真正的技术主线
```

Kubernetes 可以作为最终扩展项，但不是核心验收条件。

---

# 41. 项目为什么不算“玩具”

如果只做到：

```text
Master
+
Worker
+
LRU
+
Hash
```

仍然属于普通学生项目。

本项目最终要求：

```text
Control / Data Plane Separation
+
Object / Chunk
+
ReadPlan
+
RAM / NVMe Tiered Cache
+
Capacity-aware Placement
+
Replication / Repair
+
Failure Detection
+
Prefetch
+
Hedged Read
+
ReadPlan Cache
+
Backpressure
+
Snapshot Recovery
+
Linux IO Optimization
+
C++ Artifact API
+
MySQL / Redis / Kafka / Outbox
+
Docker Compose
+
Prometheus / Grafana
+
Real Benchmark
```

因此更准确的定义是：

> **可部署、可验证、可实际使用的分布式存储系统原型。**

不声称“生产级”，因为：

- 没有几十/几百台真实机器；
- 没有长期生产流量；
- 没有多故障域；
- 没有真实大规模生产运维验证。

---

# 42. 不同岗位的面试讲法

## 42.1 C++ Linux 后端

重点：

```text
C++ API Server
线程池
连接池
HTTP/RPC
MySQL
Redis
Kafka
Transactional Outbox
异步 Job
幂等
限流
Docker
Prometheus
```

底层自研存储作为额外亮点。

---

## 42.2 C++ 存储 / 基础架构

重点：

```text
Control / Data Plane
ReadPlan
Chunk
Placement
Replication
Failure Recovery
RAM/NVMe Cache
Prefetch
Hedged Read
io_uring
perf / strace
```

业务层只简单介绍。

---

## 42.3 AI Storage / AI Infra

重点：

```text
Model Artifact
Dataset
Checkpoint

RAM/NVMe Tiered Cache

ReadPlan
Prefetch
Hotspot
Backend Traffic Reduction
Model/Data Loading Latency
```

如果未来需要再向 GPU 场景延伸，可以增加：

```text
vLLM / LMCache integration demo
```

但不作为第一阶段必需项。

---

# 43. 项目技术画像

完成后项目覆盖：

## C++

```text
C++17/20
RAII
Concurrency
Memory Management
Thread Pool
Buffer Pool
Networking
RPC
Async IO
```

## Linux

```text
Socket
File IO
epoll / io_uring
sendfile
Filesystem
Page Cache
perf
strace
iostat
pidstat
```

## Storage

```text
Object
Chunk
Replica
Placement
Tiered Cache
Eviction
Admission
Prefetch
Failure Recovery
Metadata
```

## Distributed System

```text
Membership
Heartbeat
Failure Detector
Replication
Repair
ReadPlan
Idempotency
Backpressure
Retry
```

## Backend

```text
HTTP
MySQL
Redis
Kafka
Transactional Outbox
Async Job
Nginx
```

## Engineering

```text
Docker Compose
Prometheus
Grafana
Benchmark
Fault Injection
Testing
CMake
```

---

# 44. 推荐仓库结构

最终可以逐步收敛到：

```text
project/
├── CMakeLists.txt
├── cmake/
├── configs/
├── proto/
│
├── include/
│   └── project/
│
├── src/
│   ├── common/
│   │   ├── status/
│   │   ├── logging/
│   │   ├── config/
│   │   └── concurrency/
│   │
│   ├── storage/
│   │   ├── control/
│   │   │   ├── metadata/
│   │   │   ├── read_plan/
│   │   │   ├── placement/
│   │   │   ├── membership/
│   │   │   ├── replication/
│   │   │   └── recovery/
│   │   │
│   │   ├── worker/
│   │   │   ├── ram_cache/
│   │   │   ├── nvme_cache/
│   │   │   ├── eviction/
│   │   │   ├── io/
│   │   │   └── transfer/
│   │   │
│   │   └── client/
│   │       ├── sdk/
│   │       ├── read/
│   │       └── write/
│   │
│   ├── business/
│   │   ├── api/
│   │   ├── service/
│   │   ├── domain/
│   │   ├── repository/
│   │   └── infra/
│   │
│   ├── jobs/
│   │   ├── prefetch/
│   │   ├── gc/
│   │   ├── audit/
│   │   └── outbox/
│   │
│   └── cli/
│
├── tests/
│   ├── unit/
│   ├── integration/
│   ├── failure/
│   └── e2e/
│
├── benchmarks/
│   ├── workloads/
│   ├── scripts/
│   └── reports/
│
├── deploy/
│   ├── docker/
│   ├── compose/
│   ├── prometheus/
│   └── grafana/
│
├── docs/
│   ├── architecture/
│   ├── design/
│   ├── benchmark/
│   └── interview/
│
└── tools/
```

仓库目录不用一开始一次性全部创建，按照版本阶段逐步落地。

---

# 45. 项目风险

## 风险 1：范围爆炸

最危险。

解决：

```text
严格按照 V0 → V11
每个版本只有明确目标
不跨阶段堆技术
```

---

## 风险 2：中间件喧宾夺主

解决：

```text
70% 深度
→ 自研 C++ Storage Core

30%
→ Business / Middleware / Deployment
```

---

## 风险 3：只有架构，没有性能数据

解决：

每一个重要机制必须：

```text
Baseline
↓
Implementation
↓
Benchmark
↓
Profiler
↓
Conclusion
```

---

## 风险 4：AI 名字大于实际能力

解决：

项目名称强调：

> C++ 分布式分层缓存与数据放置系统

AI 是 workload，而不是强行声称：

> 自研 AI Storage 平台。

---

## 风险 5：单机模拟多节点导致性能结论失真

解决：

报告明确区分：

```text
Architecture / Relative Performance
→ Ubuntu VM 单机多进程

Absolute NVMe Performance
→ 不做跨机结论

未来有额外机器时
→ 再补真实多机测试
```

---

# 46. 最终项目一句话

推荐以后统一描述为：

> **基于 C++ 实现面向模型、数据集及 Checkpoint 等大对象访问场景的分布式分层缓存与数据放置系统，通过集中式控制面生成 ReadPlan、数据面直传、RAM/NVMe 分层缓存、容量感知放置、复制修复、预取与尾延迟优化降低重复后端访问；上层实现制品版本管理、异步任务、可靠事件与 Docker 化部署，并通过完整 Benchmark 与故障注入验证系统性能和恢复能力。**

---

# 47. 项目最终原则

整个项目开发过程中始终坚持：

```text
不是追求工业系统的规模
而是实现工业系统的核心机制

不是堆中间件
而是让每一个组件解决真实问题

不是只跑通 Demo
而是做到可用、可部署、可验证

不是声称生产级
而是做一个扎实的分布式存储系统原型
```

这就是后续所有设计、实现、Benchmark 和简历表达的总纲。
