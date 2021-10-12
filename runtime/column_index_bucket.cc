#include "runtime/column_index_bucket.h"

#include <cstdint>

namespace kush::runtime {

// Get the next tuple that is greater than or equal to from index or cardinality
int32_t FastForwardBucket(ColumnIndexBucket* bucket, int32_t prev_tuple) {
  if (bucket->size == 0) {
    return INT32_MAX;
  }

  // Binary search for tuple greater than prev_tuple
  int start = 0;
  int end = bucket->size - 1;

  int32_t next_greater_idx = INT32_MAX;
  while (start <= end) {
    int mid = (start + end) / 2;

    if (bucket->data[mid] < prev_tuple) {
      start = mid + 1;
    } else {
      next_greater_idx = mid;
      end = mid - 1;
    }
  }

  return next_greater_idx;
}

int32_t GetBucketValue(ColumnIndexBucket* bucket, int32_t idx) {
  return bucket->data[idx];
}

}  // namespace kush::runtime
