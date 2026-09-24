#include "storage/common/logging.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>

namespace storage {

Status InitLogging(const LoggingOptions &options) {
    const auto level = spdlog::level::from_str(options.level);
    if (level == spdlog::level::off && options.level != "off") {
        return {StatusCode::kInvalidArgument, "unknown log level: " + options.level};
    }

    std::shared_ptr<spdlog::logger> logger;
    if (options.destination == LogDestination::kStdout) {
        logger = spdlog::stdout_color_mt("storage");
    } else {
        logger = spdlog::stderr_color_mt("storage");
    }
    spdlog::set_default_logger(std::move(logger));
    spdlog::set_level(level);
    spdlog::set_pattern(options.pattern);
    return Status::Ok();
}

} // namespace storage
