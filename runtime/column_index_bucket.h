#pragma once

#include <cstdint>

namespace kush::runtime {

struct ColumnIndexBucket {
  int32_t* data;
  int32_t size;
};

int32_t FastForwardBucket(ColumnIndexBucket* bucket, int32_t prev_tuple);

int32_t GetBucketValue(ColumnIndexBucket* bucket, int32_t idx);

ColumnIndexBucket* BucketListGet(ColumnIndexBucket* bucket_list, int32_t idx);

int32_t BucketListSortedIntersection(ColumnIndexBucket* bucket_list,
                                     int32_t size, int32_t* intersection_state,
                                     int32_t* result, int32_t result_max_size);

}  // namespace kush::runtime