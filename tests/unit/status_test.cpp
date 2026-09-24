#include "storage/common/status.h"

#include <gtest/gtest.h>

namespace storage {
namespace {

TEST(StatusTest, DefaultAndFactoryAreOk) {
    const Status default_status;
    const Status factory_status = Status::Ok();
    EXPECT_TRUE(default_status.ok());
    EXPECT_TRUE(factory_status.ok());
    EXPECT_EQ(default_status.code(), StatusCode::kOk);
    EXPECT_TRUE(default_status.message().empty());
}

TEST(StatusTest, ErrorPreservesCodeAndMessage) {
    const Status status(StatusCode::kNotFound, "missing object");
    EXPECT_FALSE(status.ok());
    EXPECT_EQ(status.code(), StatusCode::kNotFound);
    EXPECT_EQ(status.message(), "missing object");
}

TEST(StatusTest, OkStatusDoesNotCarryErrorMessage) {
    const Status status(StatusCode::kOk, "ignored");
    EXPECT_TRUE(status.ok());
    EXPECT_TRUE(status.message().empty());
}

} // namespace
} // namespace storage
