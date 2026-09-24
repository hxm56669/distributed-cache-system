#pragma once

namespace storage {

enum class ObjectState { kAllocating, kWriting, kReady, kFailed, kDeleting };
enum class ReplicaState { kAllocating, kWriting, kReady, kFailed, kDeleting };
enum class WorkerState { kHealthy, kSuspect, kDead };

} // namespace storage
