#include "storage/common/unique_fd.h"

#include <gtest/gtest.h>

#include <cerrno>
#include <fcntl.h>
#include <utility>

namespace storage {
namespace {

int OpenNullDevice() { return ::open("/dev/null", O_RDONLY | O_CLOEXEC); }

TEST(UniqueFdTest, DefaultIsInvalid) {
    const UniqueFd fd;
    EXPECT_FALSE(fd.valid());
    EXPECT_EQ(fd.get(), -1);
}

TEST(UniqueFdTest, DestructorClosesDescriptor) {
    const int raw_fd = OpenNullDevice();
    ASSERT_GE(raw_fd, 0);
    {
        const UniqueFd fd(raw_fd);
        EXPECT_TRUE(fd.valid());
    }
    errno = 0;
    EXPECT_EQ(::fcntl(raw_fd, F_GETFD), -1);
    EXPECT_EQ(errno, EBADF);
}

TEST(UniqueFdTest, MoveInvalidatesSource) {
    UniqueFd source(OpenNullDevice());
    ASSERT_TRUE(source.valid());
    const int raw_fd = source.get();
    UniqueFd destination(std::move(source));
    EXPECT_FALSE(source.valid());
    EXPECT_EQ(destination.get(), raw_fd);
}

TEST(UniqueFdTest, MoveAssignmentClosesOldDescriptor) {
    UniqueFd source(OpenNullDevice());
    UniqueFd destination(OpenNullDevice());
    ASSERT_TRUE(source.valid());
    ASSERT_TRUE(destination.valid());
    const int old_destination = destination.get();
    destination = std::move(source);
    errno = 0;
    EXPECT_EQ(::fcntl(old_destination, F_GETFD), -1);
    EXPECT_EQ(errno, EBADF);
    EXPECT_FALSE(source.valid());
}

TEST(UniqueFdTest, ReleaseDoesNotCloseDescriptor) {
    UniqueFd fd(OpenNullDevice());
    const int raw_fd = fd.release();
    EXPECT_FALSE(fd.valid());
    EXPECT_NE(::fcntl(raw_fd, F_GETFD), -1);
    EXPECT_EQ(::close(raw_fd), 0);
}

TEST(UniqueFdTest, ResetClosesOldDescriptor) {
    UniqueFd fd(OpenNullDevice());
    const int old_fd = fd.get();
    const int replacement = OpenNullDevice();
    ASSERT_GE(replacement, 0);
    fd.reset(replacement);
    errno = 0;
    EXPECT_EQ(::fcntl(old_fd, F_GETFD), -1);
    EXPECT_EQ(errno, EBADF);
    EXPECT_EQ(fd.get(), replacement);
}

} // namespace
} // namespace storage
