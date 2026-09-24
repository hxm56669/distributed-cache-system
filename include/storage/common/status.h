#pragma once

#include <string>

namespace storage {

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

class Status {
  public:
    Status();
    Status(StatusCode code, std::string message);

    static Status Ok();

    [[nodiscard]] bool ok() const noexcept;
    [[nodiscard]] StatusCode code() const noexcept;
    [[nodiscard]] const std::string &message() const noexcept;

  private:
    StatusCode code_;
    std::string message_;
};

} // namespace storage
