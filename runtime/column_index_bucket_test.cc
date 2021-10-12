
#include "runtime/column_index_bucket.h"

#include "gtest/gtest.h"

using namespace kush::runtime;

TEST(ColumnIndexTest, FastForwardIndex) {
  std::vector<int> data{0, 4, 5, 8, 9};
  ColumnIndexBucket bucket{
      .data = data.data(),
      .size = (int)data.size(),
  };

  EXPECT_EQ(FastForwardBucket(&bucket, 0), 0);
  EXPECT_EQ(FastForwardBucket(&bucket, 1), 1);
  EXPECT_EQ(FastForwardBucket(&bucket, 2), 1);
  EXPECT_EQ(FastForwardBucket(&bucket, 3), 1);
  EXPECT_EQ(FastForwardBucket(&bucket, 4), 1);
  EXPECT_EQ(FastForwardBucket(&bucket, 5), 2);
  EXPECT_EQ(FastForwardBucket(&bucket, 6), 3);
  EXPECT_EQ(FastForwardBucket(&bucket, 7), 3);
  EXPECT_EQ(FastForwardBucket(&bucket, 8), 3);
  EXPECT_EQ(FastForwardBucket(&bucket, 9), 4);
  EXPECT_EQ(FastForwardBucket(&bucket, 10), INT32_MAX);
  EXPECT_EQ(FastForwardBucket(&bucket, 11), INT32_MAX);
}