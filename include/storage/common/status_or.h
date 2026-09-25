#pragma once

#include "storage/common/status.h"

#include <cassert>
// 提供 assert()。
// 后面的 value() 会使用：
// assert(ok());
// 如果 StatusOr 里面没有值，你却强行调用 value()，
// Debug 模式下程序会直接报错。
#include <concepts>
// 提供 C++20 concepts。
// 这里主要用到：
// std::same_as
// std::constructible_from
#include <optional>
#include <type_traits>
// 提供一些类型判断工具。
// 这里主要使用：
// std::remove_cvref_t
// std::is_nothrow_move_constructible_v
// std::is_nothrow_move_assignable_v
#include <utility>
// std::move
// std::forward

namespace storage {

template <typename T> class StatusOr {
  public:
    StatusOr(const Status &status) : status_(NormalizeError(status)) {}
    StatusOr(Status &&status) : status_(NormalizeError(std::move(status))) {}
    // template <typename U = T>
    //
    // 表示这个构造函数自己还是一个模板。
    //
    // U 是实际传进来的值的类型。
    //
    // T 是 StatusOr 最终想保存的类型。
    //
    // 比如：
    //
    // StatusOr<std::string> x("hello");
    //
    // 此时：
    //
    // T = std::string
    // U 可能推导为 const char[6]
    //
    // 只要 std::string 能用这个 U 构造出来，
    // 就允许调用这个构造函数。

    // requires 是 C++20 的“约束”。
        //
        // 只有下面两个条件都成立，
        // 这个构造函数才存在。

        // remove_cvref_t<U>
        //
        // 会去掉：
        //
        // const
        // volatile
        // &
        // &&

        // T 必须能够使用 U&& 构造。
        //
        // 例如：
        //
        // T = std::string
        // U = const char*
        //
        // std::string 可以由 const char* 构造，
        // 所以合法。
        //
        // 又例如：
        //
        // T = std::string
        // U = int
        //
        // std::string 不能直接通过 int 构造，
        // 那么这个构造函数就不能使用。
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
