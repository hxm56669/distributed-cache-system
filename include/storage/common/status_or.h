#pragma once

#include "storage/common/status.h"

#include <cassert>
#include <concepts>
#include <optional>
#include <type_traits>
#include <utility>

namespace storage {

template <typename T> class StatusOr {
  public:
    StatusOr(const Status &status) : status_(NormalizeError(status)) {}
    StatusOr(Status &&status) : status_(NormalizeError(std::move(status))) {}

    template <typename U = T>
        requires(!std::same_as<std::remove_cvref_t<U>, StatusOr> &&
                 std::constructible_from<T, U &&>)
    StatusOr(U &&value) : status_(Status::Ok()), value_(std::forward<U>(value)) {}

    StatusOr(const StatusOr &) = default;
    StatusOr &operator=(const StatusOr &) = default;
    StatusOr(StatusOr &&) noexcept(std::is_nothrow_move_constructible_v<T>) = default;
    StatusOr &operator=(StatusOr &&) noexcept(std::is_nothrow_move_assignable_v<T>) = default;

    [[nodiscard]] bool ok() const noexcept { return value_.has_value(); }
    [[nodiscard]] const Status &status() const noexcept { return status_; }

    T &value() & {
        assert(ok());
        return *value_;
    }
    const T &value() const & {
        assert(ok());
        return *value_;
    }
    T &&value() && {
        assert(ok());
        return std::move(*value_);
    }

  private:
    static Status NormalizeError(Status status) {
        if (status.ok()) {
            return {StatusCode::kInternal, "StatusOr cannot be constructed from an OK status"};
        }
        return status;
    }

    Status status_;
    std::optional<T> value_;
};

} // namespace storage
