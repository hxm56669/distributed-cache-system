#include "storage/storage/types/byte_range.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <limits>

namespace storage {
namespace {

TEST(ByteRangeTest, AcceptsNormalAndBoundaryRanges) {
    EXPECT_TRUE(ValidateByteRange({10, 20}, 100).ok());
    EXPECT_TRUE(ValidateByteRange({80, 20}, 100).ok());
    EXPECT_TRUE(ValidateByteRange({100, 0}, 100).ok());
    EXPECT_TRUE(ValidateByteRange({0, 0}, 0).ok());
}

TEST(ByteRangeTest, RejectsOffsetBeyondTotalSize) {
    EXPECT_EQ(ValidateByteRange({101, 0}, 100).code(), StatusCode::kInvalidArgument);
}

TEST(ByteRangeTest, RejectsSizeBeyondBoundary) {
    EXPECT_EQ(ValidateByteRange({80, 21}, 100).code(), StatusCode::kInvalidArgument);
}

TEST(ByteRangeTest, RejectsOverflowingSum) {
    constexpr auto kMax = std::numeric_limits<std::uint64_t>::max();
    EXPECT_EQ(ValidateByteRange({kMax - 5, 10}, kMax).code(), StatusCode::kInvalidArgument);
}

} // namespace
} // namespace storage
