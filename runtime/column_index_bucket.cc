#include "runtime/column_index_bucket.h"

#include <cstdint>
#include <functional>
#include <iostream>
#include <queue>
#include <utility>
#include <vector>

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

ColumnIndexBucket* BucketListGet(ColumnIndexBucket* bucket, int32_t idx) {
  return &bucket[idx];
}

void BucketListSortedIntersectionInit(ColumnIndexBucket* bucket_list,
                                      int32_t size, int32_t* intersection_state,
                                      int32_t next_tuple) {
  for (int i = 0; i < size; i++) {
    intersection_state[i] = FastForwardBucket(&bucket_list[i], next_tuple);
  }
}

int32_t BucketListSortedIntersectionPopulateResult(
    ColumnIndexBucket* bucket_list, int32_t bucket_list_size,
    int32_t* intersection_state, int32_t* result, int32_t result_max_size) {
  if (bucket_list_size == 0) {
    return 0;
  }

  if (bucket_list_size == 1) {
    int32_t result_size = 0;
    while (intersection_state[0] < bucket_list[0].size &&
           result_size < result_max_size) {
      result[result_size++] = bucket_list[0].data[intersection_state[0]++];
    }
    return result_size;
  }

  for (int i = 0; i < bucket_list_size; i++) {
    if (intersection_state[i] >= bucket_list[i].size) {
      return 0;
    }
  }

  // find the minimum list
  int min_list = 0;
  for (int i = 1; i < bucket_list_size; i++) {
    auto size = bucket_list[i].size - intersection_state[i];
    auto min_list_size =
        bucket_list[min_list].size - intersection_state[min_list];

    if (size < min_list_size) {
      min_list = i;
    }
  }

  // scan over min list
  int32_t result_size = 0;
  for (; intersection_state[min_list] < bucket_list[min_list].size;
       intersection_state[min_list]++) {
    auto idx = intersection_state[min_list];
    if (result_size == result_max_size) {
      break;
    }

    int32_t candidate = bucket_list[min_list].data[idx];

    // see if this is contained in side each bucket list
    bool all_equal = true;
    for (int i = 0; i < bucket_list_size; i++) {
      if (i == min_list) {
        continue;
      }

      intersection_state[i] = FastForwardBucket(&bucket_list[i], candidate);
      if (intersection_state[i] >= bucket_list[i].size) {
        return result_size;
      }

      if (bucket_list[i].data[intersection_state[i]] != candidate) {
        all_equal = false;
        break;
      }
    }

    if (all_equal) {
      result[result_size++] = candidate;
    }
  }

  return result_size;
}

int32_t BucketListSortedIntersectionResultGet(int32_t* result, int32_t idx) {
  return result[idx];
}

}  // namespace kush::runtime
