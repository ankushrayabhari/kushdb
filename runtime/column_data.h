#pragma once

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "runtime/string.h"

namespace kush::data {

struct Int8ColumnData {
  int8_t* data;
  uint32_t file_length;
};

struct Int16ColumnData {
  int16_t* data;
  uint32_t file_length;
};

struct Int32ColumnData {
  int32_t* data;
  uint32_t file_length;
};

struct Int64ColumnData {
  int64_t* data;
  uint32_t file_length;
};

struct Float64ColumnData {
  double* data;
  uint32_t file_length;
};

struct StringMetadata {
  uint32_t cardinality;
  struct {
    uint32_t length;
    uint32_t offset;
  } slot[];
};

struct TextColumnData {
  StringMetadata* data;
  uint32_t file_length;
};

// Open column data file
void Open(Int8ColumnData* col, const char* path);
void Open(Int16ColumnData* col, const char* path);
void Open(Int32ColumnData* col, const char* path);
void Open(Int64ColumnData* col, const char* path);
void Open(Float64ColumnData* col, const char* path);
void Open(TextColumnData* col, const char* path);

// Close column data file
void Close(Int8ColumnData* col);
void Close(Int16ColumnData* col);
void Close(Int32ColumnData* col);
void Close(Int64ColumnData* col);
void Close(Float64ColumnData* col);
void Close(TextColumnData* col);

// Size implementations
uint32_t Size(Int8ColumnData* col);
uint32_t Size(Int16ColumnData* col);
uint32_t Size(Int32ColumnData* col);
uint32_t Size(Int64ColumnData* col);
uint32_t Size(Float64ColumnData* col);
uint32_t Size(TextColumnData* col);

// Get the ith element of the column
int8_t Get(Int8ColumnData* col, uint32_t idx);
int16_t Get(Int16ColumnData* col, uint32_t idx);
int32_t Get(Int32ColumnData* col, uint32_t idx);
int64_t Get(Int64ColumnData* col, uint32_t idx);
double Get(Float64ColumnData* col, uint32_t idx);
void Get(TextColumnData* col, uint32_t idx, String* dest);

// Create the column index
std::unordered_map<int8_t, std::vector<uint32_t>>* CreateInt8Index();
std::unordered_map<int16_t, std::vector<uint32_t>>* CreateInt16Index();
std::unordered_map<int32_t, std::vector<uint32_t>>* CreateInt32Index();
std::unordered_map<int64_t, std::vector<uint32_t>>* CreateInt64Index();
std::unordered_map<double, std::vector<uint32_t>>* CreateFloat64Index();
std::unordered_map<std::string, std::vector<uint32_t>>* CreateStringIndex();

// Free the column index
void Free(std::unordered_map<int8_t, std::vector<uint32_t>>* index);
void Free(std::unordered_map<int16_t, std::vector<uint32_t>>* index);
void Free(std::unordered_map<int32_t, std::vector<uint32_t>>* index);
void Free(std::unordered_map<int64_t, std::vector<uint32_t>>* index);
void Free(std::unordered_map<double, std::vector<uint32_t>>* index);
void Free(std::unordered_map<std::string, std::vector<uint32_t>>* index);

// Insert tuple idx
void Insert(std::unordered_map<int8_t, std::vector<uint32_t>>* index,
            int8_t value, uint32_t tuple_idx);
void Insert(std::unordered_map<int16_t, std::vector<uint32_t>>* index,
            int16_t value, uint32_t tuple_idx);
void Insert(std::unordered_map<int32_t, std::vector<uint32_t>>* index,
            int32_t value, uint32_t tuple_idx);
void Insert(std::unordered_map<int64_t, std::vector<uint32_t>>* index,
            int64_t value, uint32_t tuple_idx);
void Insert(std::unordered_map<double, std::vector<uint32_t>>* index,
            double value, uint32_t tuple_idx);
void Insert(std::unordered_map<std::string, std::vector<uint32_t>>* index,
            String* value, uint32_t tuple_idx);

// Get the next tuple from index
uint32_t GetNextTuple(std::unordered_map<int8_t, std::vector<uint32_t>>* index,
                      int8_t value, uint32_t prev_tuple, uint32_t cardinality);
uint32_t GetNextTuple(std::unordered_map<int16_t, std::vector<uint32_t>>* index,
                      int16_t value, uint32_t prev_tuple, uint32_t cardinality);
uint32_t GetNextTuple(std::unordered_map<int32_t, std::vector<uint32_t>>* index,
                      int32_t value, uint32_t prev_tuple, uint32_t cardinality);
uint32_t GetNextTuple(std::unordered_map<int64_t, std::vector<uint32_t>>* index,
                      int64_t value, uint32_t prev_tuple, uint32_t cardinality);
uint32_t GetNextTuple(std::unordered_map<double, std::vector<uint32_t>>* index,
                      double value, uint32_t prev_tuple, uint32_t cardinality);
uint32_t GetNextTuple(
    std::unordered_map<std::string, std::vector<uint32_t>>* index,
    String* value, uint32_t prev_tuple, uint32_t cardinality);

}  // namespace kush::data