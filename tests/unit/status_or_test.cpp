#include "storage/common/status_or.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>

namespace storage {
namespace {

TEST(StatusOrTest, HoldsSuccessfulValue) {
    StatusOr<std::string> result(std::string("value"));
    ASSERT_TRUE(result.ok());
    EXPECT_TRUE(result.status().ok());
    EXPECT_EQ(result.value(), "value");
}

TEST(StatusOrTest, HoldsErrorStatus) {
    StatusOr<int> result(Status(StatusCode::kUnavailable, "offline"));
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.status().code(), StatusCode::kUnavailable);
    EXPECT_EQ(result.status().message(), "offline");
}

TEST(StatusOrTest, RejectsOkStatusWithoutValue) {
    StatusOr<int> result(Status::Ok());
    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.status().code(), StatusCode::kInternal);
}

TEST(StatusOrTest, SupportsMoveOnlyValueAndMoveConstruction) {
    StatusOr<std::unique_ptr<int>> original(std::make_unique<int>(42));
    StatusOr<std::unique_ptr<int>> moved(std::move(original));
    ASSERT_TRUE(moved.ok());
    EXPECT_EQ(*moved.value(), 42);
}

TEST(StatusOrTest, SupportsMoveAssignment) {
    StatusOr<std::unique_ptr<int>> source(std::make_unique<int>(7));
    StatusOr<std::unique_ptr<int>> destination(Status(StatusCode::kInternal, "not initialized"));
    destination = std::move(source);
    ASSERT_TRUE(destination.ok());
    EXPECT_EQ(*destination.value(), 7);
}

TEST(StatusOrTest, RvalueValueTransfersOwnership) {
    StatusOr<std::unique_ptr<int>> result(std::make_unique<int>(9));
    auto value = std::move(result).value();
    ASSERT_NE(value, nullptr);
    EXPECT_EQ(*value, 9);
}

} // namespace
} // namespace storage
