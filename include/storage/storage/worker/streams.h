#pragma once

#include "storage/common/status_or.h"

#include <cstddef>
#include <span>

namespace storage {

class ChunkInputStream {
  public:
    virtual ~ChunkInputStream() = default;
    // A successful zero-byte read is EOF. Empty input buffers are invalid.
    virtual StatusOr<std::size_t> Read(std::span<std::byte> buffer) = 0;
};

class ChunkOutputStream {
  public:
    virtual ~ChunkOutputStream() = default;
    virtual Status Write(std::span<const std::byte> data) = 0;
};

} // namespace storage
