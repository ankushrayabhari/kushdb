#include "runtime/column_data.h"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <vector>

namespace kush::data {

// ------ Open --------

template <typename T>
inline void OpenImpl(T*& data, int32_t& file_length, const char* path) {
  int fd = open(path, O_RDONLY);
  if (fd == -1) {
    throw std::system_error(errno, std::generic_category());
  }

  struct stat sb;
  if (fstat(fd, &sb) == -1) {
    throw std::system_error(errno, std::generic_category());
  }
  file_length = static_cast<int32_t>(sb.st_size);

  data = reinterpret_cast<T*>(
      mmap(nullptr, file_length, PROT_READ, MAP_PRIVATE, fd, 0));
  if (data == MAP_FAILED) {
    throw std::system_error(errno, std::generic_category());
  }

  close(fd);
}

void Open(Int8ColumnData* col, const char* path) {
  OpenImpl(col->data, col->file_length, path);
}

void Open(Int16ColumnData* col, const char* path) {
  OpenImpl(col->data, col->file_length, path);
}

void Open(Int32ColumnData* col, const char* path) {
  OpenImpl(col->data, col->file_length, path);
}

void Open(Int64ColumnData* col, const char* path) {
  OpenImpl(col->data, col->file_length, path);
}

void Open(Float64ColumnData* col, const char* path) {
  OpenImpl(col->data, col->file_length, path);
}

void Open(TextColumnData* col, const char* path) {
  OpenImpl(col->data, col->file_length, path);
}

// ------ Close --------

template <typename T>
inline void CloseImpl(T* column) {
  if (column->data) {
    munmap(column->data, column->file_length);
  }
}

void Close(Int8ColumnData* col) { CloseImpl(col); }

void Close(Int16ColumnData* col) { CloseImpl(col); }

void Close(Int32ColumnData* col) { CloseImpl(col); }

void Close(Int64ColumnData* col) { CloseImpl(col); }

void Close(Float64ColumnData* col) { CloseImpl(col); }

void Close(TextColumnData* col) { CloseImpl(col); }

// ------ Size --------

int32_t Size(Int8ColumnData* col) { return col->file_length / sizeof(int16_t); }

int32_t Size(Int16ColumnData* col) {
  return col->file_length / sizeof(int16_t);
}

int32_t Size(Int32ColumnData* col) {
  return col->file_length / sizeof(int32_t);
}

int32_t Size(Int64ColumnData* col) {
  return col->file_length / sizeof(int64_t);
}

int32_t Size(Float64ColumnData* col) {
  return col->file_length / sizeof(double);
}

int32_t Size(TextColumnData* col) { return col->data->cardinality; }

// ------ Get --------

int8_t Get(Int8ColumnData* col, int32_t idx) { return col->data[idx]; }

int16_t Get(Int16ColumnData* col, int32_t idx) { return col->data[idx]; }

int32_t Get(Int32ColumnData* col, int32_t idx) { return col->data[idx]; }

int64_t Get(Int64ColumnData* col, int32_t idx) { return col->data[idx]; }

double Get(Float64ColumnData* col, int32_t idx) { return col->data[idx]; }

void Get(TextColumnData* col, int32_t idx, String* dest) {
  const auto& slot = col->data->slot[idx];
  dest->data = reinterpret_cast<const char*>(col->data) + slot.offset;
  dest->length = slot.length;
}

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

std::unordered_map<std::string, std::vector<int32_t>>* CreateStringIndex() {
  return new std::unordered_map<std::string, std::vector<int32_t>>;
}

// ---------- Free Index -------------
void Free(std::unordered_map<int8_t, std::vector<int32_t>>* index) {
  delete index;
}

void Free(std::unordered_map<int16_t, std::vector<int32_t>>* index) {
  delete index;
}

void Free(std::unordered_map<int32_t, std::vector<int32_t>>* index) {
  delete index;
}

void Free(std::unordered_map<int64_t, std::vector<int32_t>>* index) {
  delete index;
}

void Free(std::unordered_map<double, std::vector<int32_t>>* index) {
  delete index;
}

void Free(std::unordered_map<std::string, std::vector<int32_t>>* index) {
  delete index;
}

// ---------- Insert -------------
void Insert(std::unordered_map<int8_t, std::vector<int32_t>>* index,
            int8_t value, int32_t tuple_idx) {
  index->operator[](value).push_back(tuple_idx);
}

void Insert(std::unordered_map<int16_t, std::vector<int32_t>>* index,
            int16_t value, int32_t tuple_idx) {
  index->operator[](value).push_back(tuple_idx);
}

void Insert(std::unordered_map<int32_t, std::vector<int32_t>>* index,
            int32_t value, int32_t tuple_idx) {
  index->operator[](value).push_back(tuple_idx);
}

void Insert(std::unordered_map<int64_t, std::vector<int32_t>>* index,
            int64_t value, int32_t tuple_idx) {
  index->operator[](value).push_back(tuple_idx);
}

void Insert(std::unordered_map<double, std::vector<int32_t>>* index,
            double value, int32_t tuple_idx) {
  index->operator[](value).push_back(tuple_idx);
}

void Insert(std::unordered_map<std::string, std::vector<int32_t>>* index,
            String* value, int32_t tuple_idx) {
  index->operator[](std::string(value->data, value->length))
      .push_back(tuple_idx);
}

// Get the next tuple from index or cardinality
template <typename T>
inline int32_t GetNextTupleImpl(
    std::unordered_map<T, std::vector<int32_t>>* index, const T& value,
    int32_t prev_tuple, int32_t cardinality) {
  // Get bucket
  const auto& bucket = index->at(value);

  // Binary search for tuple greater than prev_tuple
  int start = 0;
  int end = bucket.size() - 1;

  int32_t next_greater = cardinality;
  while (start <= end) {
    int mid = (start + end) / 2;

    if (bucket[mid] <= prev_tuple) {
      start = mid + 1;
    } else {
      next_greater = mid;
      end = mid - 1;
    }
  }

  return next_greater;
}

int32_t GetNextTuple(std::unordered_map<int8_t, std::vector<int32_t>>* index,
                     int8_t value, int32_t prev_tuple, int32_t cardinality) {
  return GetNextTupleImpl(index, value, prev_tuple, cardinality);
}

int32_t GetNextTuple(std::unordered_map<int16_t, std::vector<int32_t>>* index,
                     int16_t value, int32_t prev_tuple, int32_t cardinality) {
  return GetNextTupleImpl(index, value, prev_tuple, cardinality);
}

int32_t GetNextTuple(std::unordered_map<int32_t, std::vector<int32_t>>* index,
                     int32_t value, int32_t prev_tuple, int32_t cardinality) {
  return GetNextTupleImpl(index, value, prev_tuple, cardinality);
}

int32_t GetNextTuple(std::unordered_map<int64_t, std::vector<int32_t>>* index,
                     int64_t value, int32_t prev_tuple, int32_t cardinality) {
  return GetNextTupleImpl(index, value, prev_tuple, cardinality);
}

int32_t GetNextTuple(std::unordered_map<double, std::vector<int32_t>>* index,
                     double value, int32_t prev_tuple, int32_t cardinality) {
  return GetNextTupleImpl(index, value, prev_tuple, cardinality);
}

int32_t GetNextTuple(
    std::unordered_map<std::string, std::vector<int32_t>>* index, String* value,
    int32_t prev_tuple, int32_t cardinality) {
  return GetNextTupleImpl(index, std::string(value->data, value->length),
                          prev_tuple, cardinality);
}

}  // namespace kush::data