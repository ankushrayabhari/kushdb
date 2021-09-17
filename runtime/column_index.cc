#include "runtime/column_index.h"

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

namespace kush::runtime::ColumnIndex {

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
inline std::vector<int32_t>* GetBucketImpl(
    absl::flat_hash_map<T, std::vector<int32_t>>* index, S value) {
  auto it = index->find(value);

  if (it == index->end()) {
    return nullptr;
  }

  // Get bucket
  return &it->second;
}

std::vector<int32_t>* GetBucketInt8Index(
    absl::flat_hash_map<int32_t, std::vector<int32_t>>* index, int8_t value) {
  return GetBucketImpl(index, value);
}

std::vector<int32_t>* GetBucketInt16Index(
    absl::flat_hash_map<int32_t, std::vector<int32_t>>* index, int16_t value) {
  return GetBucketImpl(index, value);
}

std::vector<int32_t>* GetBucketInt32Index(
    absl::flat_hash_map<int32_t, std::vector<int32_t>>* index, int32_t value) {
  return GetBucketImpl(index, value);
}

std::vector<int32_t>* GetBucketInt64Index(
    absl::flat_hash_map<int64_t, std::vector<int32_t>>* index, int64_t value) {
  return GetBucketImpl(index, value);
}

std::vector<int32_t>* GetBucketFloat64Index(
    absl::flat_hash_map<double, std::vector<int32_t>>* index, double value) {
  return GetBucketImpl(index, value);
}

std::vector<int32_t>* GetBucketTextIndex(
    absl::flat_hash_map<std::string, std::vector<int32_t>>* index,
    String::String* value) {
  return GetBucketImpl(index, std::string_view(value->data, value->length));
}

std::vector<std::vector<int32_t>*>* CreateBucketList() {
  return new std::vector<std::vector<int32_t>*>();
}

int32_t BucketListSize(std::vector<std::vector<int32_t>*>* bucket_list) {
  return bucket_list->size();
}

void BucketListPushBack(std::vector<std::vector<int32_t>*>* bucket_list,
                        std::vector<int32_t>* bucket) {
  bucket_list->push_back(bucket);
}

std::vector<int32_t>* BucketListGet(
    std::vector<std::vector<int32_t>*>* bucket_list, int32_t idx) {
  return bucket_list->at(idx);
}

void FreeBucketList(std::vector<std::vector<int32_t>*>* bucket_list) {
  delete bucket_list;
}

}  // namespace kush::runtime::ColumnIndex