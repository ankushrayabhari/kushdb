
#include "gtest/gtest.h"

#include "runtime/memory_column_index.h"

using namespace kush::runtime;

TEST(ColumnIndexTest, FastForwardIndex) {
  std::vector<int> bucket{0, 4, 5, 8, 9};
  EXPECT_EQ(MemoryColumnIndex::FastForwardBucket(&bucket, 0), 0);
  EXPECT_EQ(MemoryColumnIndex::FastForwardBucket(&bucket, 1), 1);
  EXPECT_EQ(MemoryColumnIndex::FastForwardBucket(&bucket, 2), 1);
  EXPECT_EQ(MemoryColumnIndex::FastForwardBucket(&bucket, 3), 1);
  EXPECT_EQ(MemoryColumnIndex::FastForwardBucket(&bucket, 4), 1);
  EXPECT_EQ(MemoryColumnIndex::FastForwardBucket(&bucket, 5), 2);
  EXPECT_EQ(MemoryColumnIndex::FastForwardBucket(&bucket, 6), 3);
  EXPECT_EQ(MemoryColumnIndex::FastForwardBucket(&bucket, 7), 3);
  EXPECT_EQ(MemoryColumnIndex::FastForwardBucket(&bucket, 8), 3);
  EXPECT_EQ(MemoryColumnIndex::FastForwardBucket(&bucket, 9), 4);
  EXPECT_EQ(MemoryColumnIndex::FastForwardBucket(&bucket, 10), INT32_MAX);
  EXPECT_EQ(MemoryColumnIndex::FastForwardBucket(&bucket, 11), INT32_MAX);
}