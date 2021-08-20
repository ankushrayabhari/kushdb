
#include "runtime/column_index.h"

#include <gtest/gtest.h>

using namespace kush::runtime;

TEST(ColumnIndexTest, FastForwardIndex) {
  std::vector<int> bucket{0, 4, 5, 8, 9};
  EXPECT_EQ(ColumnIndex::FastForwardBucket(&bucket, 0), 0);
  EXPECT_EQ(ColumnIndex::FastForwardBucket(&bucket, 1), 1);
  EXPECT_EQ(ColumnIndex::FastForwardBucket(&bucket, 2), 1);
  EXPECT_EQ(ColumnIndex::FastForwardBucket(&bucket, 3), 1);
  EXPECT_EQ(ColumnIndex::FastForwardBucket(&bucket, 4), 1);
  EXPECT_EQ(ColumnIndex::FastForwardBucket(&bucket, 5), 2);
  EXPECT_EQ(ColumnIndex::FastForwardBucket(&bucket, 6), 3);
  EXPECT_EQ(ColumnIndex::FastForwardBucket(&bucket, 7), 3);
  EXPECT_EQ(ColumnIndex::FastForwardBucket(&bucket, 8), 3);
  EXPECT_EQ(ColumnIndex::FastForwardBucket(&bucket, 9), 4);
  EXPECT_EQ(ColumnIndex::FastForwardBucket(&bucket, 10), INT32_MAX);
  EXPECT_EQ(ColumnIndex::FastForwardBucket(&bucket, 11), INT32_MAX);
}