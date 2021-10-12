#pragma once

#include <cstdint>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

#include "absl/container/flat_hash_map.h"

#include "runtime/column_index_bucket.h"
#include "runtime/string.h"

namespace kush::runtime::MemoryColumnIndex {

// Create the column index
absl::flat_hash_map<int32_t, std::vector<int32_t>>* CreateInt8Index();
absl::flat_hash_map<int32_t, std::vector<int32_t>>* CreateInt16Index();
absl::flat_hash_map<int32_t, std::vector<int32_t>>* CreateInt32Index();
absl::flat_hash_map<int64_t, std::vector<int32_t>>* CreateInt64Index();
absl::flat_hash_map<double, std::vector<int32_t>>* CreateFloat64Index();
absl::flat_hash_map<std::string, std::vector<int32_t>>* CreateTextIndex();

// Free the column index
void FreeInt8Index(absl::flat_hash_map<int32_t, std::vector<int32_t>>* index);
void FreeInt16Index(absl::flat_hash_map<int32_t, std::vector<int32_t>>* index);
void FreeInt32Index(absl::flat_hash_map<int32_t, std::vector<int32_t>>* index);
void FreeInt64Index(absl::flat_hash_map<int64_t, std::vector<int32_t>>* index);
void FreeFloat64Index(absl::flat_hash_map<double, std::vector<int32_t>>* index);
void FreeTextIndex(
    absl::flat_hash_map<std::string, std::vector<int32_t>>* index);

// Insert tuple idx
void InsertInt8Index(absl::flat_hash_map<int32_t, std::vector<int32_t>>* index,
                     int8_t value, int32_t tuple_idx);
void InsertInt16Index(absl::flat_hash_map<int32_t, std::vector<int32_t>>* index,
                      int16_t value, int32_t tuple_idx);
void InsertInt32Index(absl::flat_hash_map<int32_t, std::vector<int32_t>>* index,
                      int32_t value, int32_t tuple_idx);
void InsertInt64Index(absl::flat_hash_map<int64_t, std::vector<int32_t>>* index,
                      int64_t value, int32_t tuple_idx);
void InsertFloat64Index(
    absl::flat_hash_map<double, std::vector<int32_t>>* index, double value,
    int32_t tuple_idx);
void InsertTextIndex(
    absl::flat_hash_map<std::string, std::vector<int32_t>>* index,
    String::String* value, int32_t tuple_idx);

// Get the bucket of tuple idx from index
void GetBucketInt8Index(
    absl::flat_hash_map<int32_t, std::vector<int32_t>>* index, int8_t value,
    ColumnIndexBucket* result);
void GetBucketInt16Index(
    absl::flat_hash_map<int32_t, std::vector<int32_t>>* index, int16_t value,
    ColumnIndexBucket* result);
void GetBucketInt32Index(
    absl::flat_hash_map<int32_t, std::vector<int32_t>>* index, int32_t value,
    ColumnIndexBucket* result);
void GetBucketInt64Index(
    absl::flat_hash_map<int64_t, std::vector<int32_t>>* index, int64_t value,
    ColumnIndexBucket* result);
void GetBucketFloat64Index(
    absl::flat_hash_map<double, std::vector<int32_t>>* index, double value,
    ColumnIndexBucket* result);
void GetBucketTextIndex(
    absl::flat_hash_map<std::string, std::vector<int32_t>>* index,
    String::String* value, ColumnIndexBucket* result);

}  // namespace kush::runtime::MemoryColumnIndex