#include "runtime/memory_column_index.h"

#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

#include "absl/container/flat_hash_map.h"

namespace kush::runtime::MemoryColumnIndex {

// ---------- Create Index -------------
absl::flat_hash_map<int32_t, std::vector<int32_t>>* CreateInt8Index() {
  return new absl::flat_hash_map<int32_t, std::vector<int32_t>>;
}

absl::flat_hash_map<int32_t, std::vector<int32_t>>* CreateInt16Index() {
  return new absl::flat_hash_map<int32_t, std::vector<int32_t>>;
}

absl::flat_hash_map<int32_t, std::vector<int32_t>>* CreateInt32Index() {
  return new absl::flat_hash_map<int32_t, std::vector<int32_t>>;
}

absl::flat_hash_map<int64_t, std::vector<int32_t>>* CreateInt64Index() {
  return new absl::flat_hash_map<int64_t, std::vector<int32_t>>;
}

absl::flat_hash_map<double, std::vector<int32_t>>* CreateFloat64Index() {
  return new absl::flat_hash_map<double, std::vector<int32_t>>;
}

absl::flat_hash_map<std::string, std::vector<int32_t>>* CreateTextIndex() {
  return new absl::flat_hash_map<std::string, std::vector<int32_t>>;
}

// ---------- Free Index -------------
void FreeInt8Index(absl::flat_hash_map<int32_t, std::vector<int32_t>>* index) {
  delete index;
}

void FreeInt16Index(absl::flat_hash_map<int32_t, std::vector<int32_t>>* index) {
  delete index;
}

void FreeInt32Index(absl::flat_hash_map<int32_t, std::vector<int32_t>>* index) {
  delete index;
}

void FreeInt64Index(absl::flat_hash_map<int64_t, std::vector<int32_t>>* index) {
  delete index;
}

void FreeFloat64Index(
    absl::flat_hash_map<double, std::vector<int32_t>>* index) {
  delete index;
}

void FreeTextIndex(
    absl::flat_hash_map<std::string, std::vector<int32_t>>* index) {
  delete index;
}

// ---------- Insert -------------
void InsertInt8Index(absl::flat_hash_map<int32_t, std::vector<int32_t>>* index,
                     int8_t value, int32_t tuple_idx) {
  index->operator[](value).push_back(tuple_idx);
}

void InsertInt16Index(absl::flat_hash_map<int32_t, std::vector<int32_t>>* index,
                      int16_t value, int32_t tuple_idx) {
  index->operator[](value).push_back(tuple_idx);
}

void InsertInt32Index(absl::flat_hash_map<int32_t, std::vector<int32_t>>* index,
                      int32_t value, int32_t tuple_idx) {
  index->operator[](value).push_back(tuple_idx);
}

void InsertInt64Index(absl::flat_hash_map<int64_t, std::vector<int32_t>>* index,
                      int64_t value, int32_t tuple_idx) {
  index->operator[](value).push_back(tuple_idx);
}

void InsertFloat64Index(
    absl::flat_hash_map<double, std::vector<int32_t>>* index, double value,
    int32_t tuple_idx) {
  index->operator[](value).push_back(tuple_idx);
}

void InsertTextIndex(
    absl::flat_hash_map<std::string, std::vector<int32_t>>* index,
    String::String* value, int32_t tuple_idx) {
  index->operator[](std::string_view(value->data, value->length))
      .push_back(tuple_idx);
}

// Get the next tuple that is greater than or equal to from index or cardinality
int32_t FastForwardBucket(std::vector<int32_t>* bucket_ptr,
                          int32_t prev_tuple) {
  if (bucket_ptr->size() == 0) {
    return INT32_MAX;
  }

  // Binary search for tuple greater than prev_tuple
  int start = 0;
  const auto& bucket = *bucket_ptr;
  int end = bucket.size() - 1;

  int32_t next_greater_idx = INT32_MAX;
  while (start <= end) {
    int mid = (start + end) / 2;

    if (bucket[mid] < prev_tuple) {
      start = mid + 1;
    } else {
      next_greater_idx = mid;
      end = mid - 1;
    }
  }

  return next_greater_idx;
}

int32_t BucketSize(std::vector<int32_t>* bucket) { return bucket->size(); }

int32_t BucketGet(std::vector<int32_t>* bucket, int32_t idx) {
  return (*bucket)[idx];
}

// Get the next tuple from index or cardinality
template <typename T, typename S>
void GetBucketImpl(absl::flat_hash_map<T, std::vector<int32_t>>* index, S value,
                   ColumnIndexBucket* result) {
  auto it = index->find(value);

  if (it == index->end()) {
    result->size = 0;
    result->data = nullptr;
    return;
  }

  result->size = it->second.size();
  result->data = it->second.data();
}

void GetBucketInt8Index(
    absl::flat_hash_map<int32_t, std::vector<int32_t>>* index, int8_t value,
    ColumnIndexBucket* result) {
  GetBucketImpl(index, value, result);
}

void GetBucketInt16Index(
    absl::flat_hash_map<int32_t, std::vector<int32_t>>* index, int16_t value,
    ColumnIndexBucket* result) {
  GetBucketImpl(index, value, result);
}

void GetBucketInt32Index(
    absl::flat_hash_map<int32_t, std::vector<int32_t>>* index, int32_t value,
    ColumnIndexBucket* result) {
  GetBucketImpl(index, value, result);
}

void GetBucketInt64Index(
    absl::flat_hash_map<int64_t, std::vector<int32_t>>* index, int64_t value,
    ColumnIndexBucket* result) {
  GetBucketImpl(index, value, result);
}

void GetBucketFloat64Index(
    absl::flat_hash_map<double, std::vector<int32_t>>* index, double value,
    ColumnIndexBucket* result) {
  GetBucketImpl(index, value, result);
}

void GetBucketTextIndex(
    absl::flat_hash_map<std::string, std::vector<int32_t>>* index,
    String::String* value, ColumnIndexBucket* result) {
  GetBucketImpl(index, std::string_view(value->data, value->length), result);
}

}  // namespace kush::runtime::MemoryColumnIndex