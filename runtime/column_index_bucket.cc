#include "runtime/column_index_bucket.h"

#include <cstdint>
#include <functional>
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

int32_t BucketListSortedIntersection(ColumnIndexBucket* bucket_list,
                                     int32_t bucket_list_size,
                                     int32_t* bucket_list_idx, int32_t* result,
                                     int32_t result_max_size) {
  using val_listidx_t = std::pair<int, int>;
  auto comp = [](const val_listidx_t& a, const val_listidx_t& b) {
    return a.first > b.first;
  };
  std::priority_queue<val_listidx_t, std::vector<val_listidx_t>, decltype(comp)>
      heap(comp);

  for (int i = 0; i < bucket_list_size; i++) {
    auto next_idx = bucket_list_idx[i];
    if (next_idx < bucket_list[i].size) {
      heap.emplace(bucket_list[i].data[next_idx], i);
    }
  }

  int32_t result_size = 0;
  while (!heap.empty() && result_size < result_max_size) {
    auto [min_val, min_list_idx] = heap.top();
    heap.pop();

    result[result_size++] = min_val;
    auto next_idx = ++bucket_list_idx[min_list_idx];
    if (next_idx < bucket_list[min_list_idx].size) {
      heap.emplace(bucket_list[min_list_idx].data[next_idx], min_list_idx);
    }
  }
  return result_size;
}

}  // namespace kush::runtime
