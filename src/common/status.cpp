#include "storage/common/status.h"

#include <utility>

namespace storage {

Status::Status() : code_(StatusCode::kOk) {}

Status::Status(StatusCode code, std::string message)
    : code_(code), message_(code == StatusCode::kOk ? std::string{} : std::move(message)) {}

Status Status::Ok() { return {}; }

bool Status::ok() const noexcept { return code_ == StatusCode::kOk; }

StatusCode Status::code() const noexcept { return code_; }

const std::string &Status::message() const noexcept { return message_; }

} // namespace storage
