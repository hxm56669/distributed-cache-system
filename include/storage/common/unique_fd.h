#pragma once

#include <unistd.h>
// Linux / POSIX 系统调用头文件。
// 这里主要是为了使用 ::close(fd)。
// close() 用来关闭文件描述符，例如普通文件、socket、pipe 等。
#include <utility>

namespace storage {

class UniqueFd {
  public:
    UniqueFd() noexcept = default;
    // explicit：禁止 int 到 UniqueFd 的隐式转换。
    // 例如：UniqueFd fd = 10;  // 不允许
    //      UniqueFd fd(10);    // 允许
    //
    // fd_(fd)：把传进来的文件描述符保存到成员变量 fd_ 中。
    // 从这一刻开始，这个 UniqueFd 对象就“拥有”这个 fd，
    // 析构时会负责 close(fd)。
    //
    // noexcept：保证这个构造函数不会抛异常。
    explicit UniqueFd(int fd) noexcept : fd_(fd) {}
    ~UniqueFd() { reset(); }

    UniqueFd(const UniqueFd &) = delete;
    UniqueFd &operator=(const UniqueFd &) = delete;

    UniqueFd(UniqueFd &&other) noexcept : fd_(other.release()) {}
    UniqueFd &operator=(UniqueFd &&other) noexcept {
        if (this != &other) {
            reset(other.release());
        }
        return *this;
    }

    [[nodiscard]] int get() const noexcept { return fd_; }
    [[nodiscard]] bool valid() const noexcept { return fd_ >= 0; }

    // release() 的含义：
    // “把 fd 的所有权交出去，但不关闭 fd。”
    //
    // std::exchange(fd_, -1) 做两件事：
    // 1. 返回 fd_ 原来的值；
    // 2. 把 fd_ 修改成 -1。
    //
    // 例如原来：
    // fd_ = 5
    //
    // 调用：
    // int fd = obj.release();
    //
    // 结果：
    // fd == 5
    // obj.fd_ == -1
    //
    // 因此 obj 之后不再拥有 fd=5，
    // 析构时也不会 close(5)。
    int release() noexcept { return std::exchange(fd_, -1); }


    // reset() 的含义：
    // “放弃当前拥有的 fd，并且可以接管一个新的 fd。”
    //
    // 默认参数 fd = -1，
    // 所以 obj.reset() 表示：
    // 只关闭当前 fd，然后变成空状态。
    void reset(int fd = -1) noexcept {
        if (fd_ >= 0 && fd_ != fd) {
            (void)::close(fd_);
        }
        fd_ = fd;
    }

  private:
    int fd_{-1};
};

} // namespace storage
