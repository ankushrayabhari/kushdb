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

void BucketListSortedIntersectionInit(ColumnIndexBucket* bucket_list,
                                      int32_t size, int32_t* intersection_state,
                                      int32_t next_tuple);

int32_t BucketListSortedIntersectionPopulateResult(
    ColumnIndexBucket* bucket_list, int32_t size, int32_t* intersection_state,
    int32_t* result, int32_t result_max_size);

int32_t BucketListSortedIntersectionPopulateResultFilter(
    ColumnIndexBucket* bucket_list, int32_t* intersection_state,
    int32_t* result, int32_t result_max_size, int* index_filter,
    int index_filter_size);

int32_t BucketListSortedIntersectionResultGet(int32_t* result, int32_t idx);

}  // namespace kush::runtime