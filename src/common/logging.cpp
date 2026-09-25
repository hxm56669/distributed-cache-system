#include "storage/common/logging.h"

#include <spdlog/sinks/stdout_color_sinks.h>
// 引入 spdlog 的 stdout/stderr 彩色日志 sink。
//
// stdout_color_mt()
// stderr_color_mt()
// 就来自这个头文件。
//
// sink 可以理解成：
// “日志最终写到哪里”。
#include <spdlog/spdlog.h>
// 引入 spdlog 的核心 API。
//
// 这里会使用：
// spdlog::set_default_logger()
// spdlog::set_level()
// spdlog::set_pattern()
// spdlog::level::from_str()
#include <memory>
// 引入智能指针。
// std::shared_ptr 就定义在这里。

namespace storage {

    // 根据 LoggingOptions 初始化整个进程的默认日志系统。
Status InitLogging(const LoggingOptions &options) {

    // 把字符串形式的日志等级转换成 spdlog 内部枚举。
    // spdlog::level::from_str() 遇到无法识别的字符串时，
    // 会得到 off。
    const auto level = spdlog::level::from_str(options.level);
    // 如果：
    //
    // level == off
    // &&
    // 原字符串 != "off"
    //
    // 就说明用户输入了非法日志等级。
    if (level == spdlog::level::off && options.level != "off") {
        return {StatusCode::kInvalidArgument, "unknown log level: " + options.level};
    }

    // 声明一个共享智能指针，用来保存 logger。
    //
    // logger 是真正的“日志器”对象。
    //
    // shared_ptr 的原因是：
    // spdlog 本身的 logger API 使用 shared_ptr 管理 logger 生命周期。
    std::shared_ptr<spdlog::logger> logger;
    if (options.destination == LogDestination::kStdout) {
        // 创建一个：
        //
        // 多线程安全 + 彩色 + stdout
        //
        // 的 logger。
        //
        // stdout
        //   -> 输出到标准输出
        //
        // color
        //   -> 不同日志等级可以显示不同颜色
        //
        // mt
        //   -> multi-threaded，多线程安全版本
        //
        // "storage"
        //   -> logger 的名字
        logger = spdlog::stdout_color_mt("storage");
    } else {
        // 与 stdout_color_mt 的主要区别只有：
        //
        // stdout_color_mt -> stdout
        // stderr_color_mt -> stderr
        logger = spdlog::stderr_color_mt("storage");
    }

    // 把刚创建的 logger 设置成 spdlog 的全局默认 logger。
    //
    // 设置以后：
    //
    // spdlog::info("hello");
    // spdlog::error("failed");
    //
    // 都会使用这个 "storage" logger。
    //
    // std::move(logger) 表示把 shared_ptr 移交给函数。
    //
    // 此后当前局部变量 logger 不再需要继续持有这一份引用。
    // 注意：
    // 移动 shared_ptr 不会移动真正的 logger 对象，
    // 只是转移 shared_ptr 本身所持有的所有权。
    spdlog::set_default_logger(std::move(logger));

    // 设置全局日志过滤等级。
    //
    // 例如 level = info：
    //
    // trace ❌
    // debug ❌
    // info  ✅
    // warn  ✅
    // error ✅
    spdlog::set_level(level);

    // 设置日志输出格式。
    spdlog::set_pattern(options.pattern);
    return Status::Ok();
}

} // namespace storage
