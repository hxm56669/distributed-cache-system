#pragma once

#include "storage/common/status.h"

#include <string>

namespace storage {

    // 不会像普通 enum 一样隐式转换成 int，也不会污染外部作用域。
enum class LogDestination { kStdout, kStderr };  // 日志输出位置

// 日志初始化配置结构体。
// LoggingOptions 使用默认成员初始化，因此可以直接：
// LoggingOptions options;
// 此时三个字段已经拥有默认值。
struct LoggingOptions {
    // 日志等级。
    // 默认值是 "info"。
    // 后续可以传入 "debug"、"warn"、"error"、"off" 等 spdlog 支持的等级。
    std::string level{"info"};
     // 日志输出格式。
    //
    // %Y -> 年
    // %m -> 月
    // %d -> 日
    // %H -> 时
    // %M -> 分
    // %S -> 秒
    // %e -> 毫秒
    // %l -> 日志等级
    // %v -> 真正的日志正文
    // [2026-09-25 16:00:12.123] [info] worker started

    std::string pattern{"[%Y-%m-%d %H:%M:%S.%e] [%l] %v"};

    // 日志输出目标。
    // 默认输出到 stderr。
    LogDestination destination{LogDestination::kStderr};
};

// 声明日志系统初始化函数。
//
// const LoggingOptions&：
// 1. 使用引用，避免复制 LoggingOptions；
// 2. 使用 const，保证函数不会修改调用者传入的配置。
Status InitLogging(const LoggingOptions &options);

} // namespace storage
