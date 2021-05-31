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

namespace kush::runtime::ColumnIndex {

// ---------- Create Index -------------
std::unordered_map<int8_t, std::vector<int32_t>>* CreateInt8Index() {
  return new std::unordered_map<int8_t, std::vector<int32_t>>;
}

std::unordered_map<int16_t, std::vector<int32_t>>* CreateInt16Index() {
  return new std::unordered_map<int16_t, std::vector<int32_t>>;
}

std::unordered_map<int32_t, std::vector<int32_t>>* CreateInt32Index() {
  return new std::unordered_map<int32_t, std::vector<int32_t>>;
}

std::unordered_map<int64_t, std::vector<int32_t>>* CreateInt64Index() {
  return new std::unordered_map<int64_t, std::vector<int32_t>>;
}

std::unordered_map<double, std::vector<int32_t>>* CreateFloat64Index() {
  return new std::unordered_map<double, std::vector<int32_t>>;
}

std::unordered_map<std::string, std::vector<int32_t>>* CreateTextIndex() {
  return new std::unordered_map<std::string, std::vector<int32_t>>;
}

// ---------- Free Index -------------
void FreeInt8Index(std::unordered_map<int8_t, std::vector<int32_t>>* index) {
  delete index;
}

void FreeInt16Index(std::unordered_map<int16_t, std::vector<int32_t>>* index) {
  delete index;
}

void FreeInt32Index(std::unordered_map<int32_t, std::vector<int32_t>>* index) {
  delete index;
}

void FreeInt64Index(std::unordered_map<int64_t, std::vector<int32_t>>* index) {
  delete index;
}

void FreeFloat64Index(std::unordered_map<double, std::vector<int32_t>>* index) {
  delete index;
}

void FreeTextIndex(
    std::unordered_map<std::string, std::vector<int32_t>>* index) {
  delete index;
}

// ---------- Insert -------------
void InsertInt8Index(std::unordered_map<int8_t, std::vector<int32_t>>* index,
                     int8_t value, int32_t tuple_idx) {
  index->operator[](value).push_back(tuple_idx);
}

void InsertInt16Index(std::unordered_map<int16_t, std::vector<int32_t>>* index,
                      int16_t value, int32_t tuple_idx) {
  index->operator[](value).push_back(tuple_idx);
}

void InsertInt32Index(std::unordered_map<int32_t, std::vector<int32_t>>* index,
                      int32_t value, int32_t tuple_idx) {
  index->operator[](value).push_back(tuple_idx);
}

void InsertInt64Index(std::unordered_map<int64_t, std::vector<int32_t>>* index,
                      int64_t value, int32_t tuple_idx) {
  index->operator[](value).push_back(tuple_idx);
}

void InsertFloat64Index(std::unordered_map<double, std::vector<int32_t>>* index,
                        double value, int32_t tuple_idx) {
  index->operator[](value).push_back(tuple_idx);
}

void InsertTextIndex(
    std::unordered_map<std::string, std::vector<int32_t>>* index,
    String::String* value, int32_t tuple_idx) {
  index->operator[](std::string(value->data, value->length))
      .push_back(tuple_idx);
}

// Get the next tuple from index or cardinality
template <typename T>
inline int32_t GetNextTupleImpl(
    std::unordered_map<T, std::vector<int32_t>>* index, const T& value,
    int32_t prev_tuple, int32_t cardinality) {
  auto it = index->find(value);

  if (it == index->end()) {
    return cardinality;
  }

  // Get bucket
  const auto& bucket = it->second;

  // Binary search for tuple greater than prev_tuple
  int start = 0;
  int end = bucket.size() - 1;

  int32_t next_greater = cardinality;
  while (start <= end) {
    int mid = (start + end) / 2;

    if (bucket[mid] <= prev_tuple) {
      start = mid + 1;
    } else {
      next_greater = bucket[mid];
      end = mid - 1;
    }
  }

  return next_greater;
}

int32_t GetNextTupleInt8Index(
    std::unordered_map<int8_t, std::vector<int32_t>>* index, int8_t value,
    int32_t prev_tuple, int32_t cardinality) {
  return GetNextTupleImpl(index, value, prev_tuple, cardinality);
}

int32_t GetNextTupleInt16Index(
    std::unordered_map<int16_t, std::vector<int32_t>>* index, int16_t value,
    int32_t prev_tuple, int32_t cardinality) {
  return GetNextTupleImpl(index, value, prev_tuple, cardinality);
}

int32_t GetNextTupleInt32Index(
    std::unordered_map<int32_t, std::vector<int32_t>>* index, int32_t value,
    int32_t prev_tuple, int32_t cardinality) {
  return GetNextTupleImpl(index, value, prev_tuple, cardinality);
}

int32_t GetNextTupleInt64Index(
    std::unordered_map<int64_t, std::vector<int32_t>>* index, int64_t value,
    int32_t prev_tuple, int32_t cardinality) {
  return GetNextTupleImpl(index, value, prev_tuple, cardinality);
}

int32_t GetNextTupleFloat64Index(
    std::unordered_map<double, std::vector<int32_t>>* index, double value,
    int32_t prev_tuple, int32_t cardinality) {
  return GetNextTupleImpl(index, value, prev_tuple, cardinality);
}

int32_t GetNextTupleTextIndex(
    std::unordered_map<std::string, std::vector<int32_t>>* index,
    String::String* value, int32_t prev_tuple, int32_t cardinality) {
  return GetNextTupleImpl(index, std::string(value->data, value->length),
                          prev_tuple, cardinality);
}

}  // namespace kush::runtime::ColumnIndex