#include "storage/storage/types/checksum.h"

#include <gtest/gtest.h>

#include <string>

namespace storage {
namespace {

TEST(ChecksumTest, AcceptsCanonicalBlake3Hex) {
    const Checksum checksum{ChecksumAlgorithm::kBlake3, std::string(64, 'a')};
    EXPECT_TRUE(ValidateChecksum(checksum).ok());
}

TEST(ChecksumTest, RejectsEmptyAndWrongLengthValues) {
    EXPECT_FALSE(ValidateChecksum({ChecksumAlgorithm::kBlake3, ""}).ok());
    EXPECT_FALSE(ValidateChecksum({ChecksumAlgorithm::kBlake3, std::string(63, '0')}).ok());
}

TEST(ChecksumTest, RejectsUppercaseAndNonHexValues) {
    EXPECT_FALSE(ValidateChecksum({ChecksumAlgorithm::kBlake3, std::string(64, 'A')}).ok());
    EXPECT_FALSE(ValidateChecksum({ChecksumAlgorithm::kBlake3, std::string(64, 'z')}).ok());
}

} // namespace
} // namespace storage
