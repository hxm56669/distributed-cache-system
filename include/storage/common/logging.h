#pragma once

#include "storage/common/status.h"

#include <string>

namespace storage {

enum class LogDestination { kStdout, kStderr };

struct LoggingOptions {
    std::string level{"info"};
    std::string pattern{"[%Y-%m-%d %H:%M:%S.%e] [%l] %v"};
    LogDestination destination{LogDestination::kStderr};
};

Status InitLogging(const LoggingOptions &options);

} // namespace storage
