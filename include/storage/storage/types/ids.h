#pragma once

#include <string>

namespace storage {

#define STORAGE_DEFINE_ID_TYPE(Name)                                                               \
    struct Name {                                                                                  \
        std::string value;                                                                         \
        [[nodiscard]] bool valid() const noexcept { return !value.empty(); }                       \
        friend bool operator==(const Name &, const Name &) = default;                              \
        friend auto operator<=>(const Name &, const Name &) = default;                             \
    }

STORAGE_DEFINE_ID_TYPE(ObjectId);
STORAGE_DEFINE_ID_TYPE(ChunkId);
STORAGE_DEFINE_ID_TYPE(ReplicaId);
STORAGE_DEFINE_ID_TYPE(WorkerId);

#undef STORAGE_DEFINE_ID_TYPE

} // namespace storage
