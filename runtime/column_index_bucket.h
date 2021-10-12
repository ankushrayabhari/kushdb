#pragma once

#include <cstdint>

namespace kush::runtime {

struct ColumnIndexBucket {
  int32_t* data;
  int32_t size;
};

int32_t FastForwardBucket(ColumnIndexBucket* bucket, int32_t prev_tuple);

int32_t GetBucketValue(ColumnIndexBucket* bucket, int32_t idx);

}  // namespace kush::runtime