#pragma once

#include "storage/common/status.h"

#include <string>

namespace storage {

enum class ChecksumAlgorithm { kBlake3 };

struct Checksum {
    ChecksumAlgorithm algorithm;
    std::string value;

    friend bool operator==(const Checksum &, const Checksum &) = default;
};

Status ValidateChecksum(const Checksum &checksum);

} // namespace storage
