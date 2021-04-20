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
  int32_t file_length;
};

struct Int16ColumnData {
  int16_t* data;
  int32_t file_length;
};

struct Int32ColumnData {
  int32_t* data;
  int32_t file_length;
};

struct Int64ColumnData {
  int64_t* data;
  int32_t file_length;
};

struct Float64ColumnData {
  double* data;
  int32_t file_length;
};

struct StringMetadata {
  int32_t cardinality;
  struct {
    int32_t length;
    int32_t offset;
  } slot[];
};

struct TextColumnData {
  StringMetadata* data;
  int32_t file_length;
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
int32_t Size(Int8ColumnData* col);
int32_t Size(Int16ColumnData* col);
int32_t Size(Int32ColumnData* col);
int32_t Size(Int64ColumnData* col);
int32_t Size(Float64ColumnData* col);
int32_t Size(TextColumnData* col);

// Get the ith element of the column
int8_t Get(Int8ColumnData* col, int32_t idx);
int16_t Get(Int16ColumnData* col, int32_t idx);
int32_t Get(Int32ColumnData* col, int32_t idx);
int64_t Get(Int64ColumnData* col, int32_t idx);
double Get(Float64ColumnData* col, int32_t idx);
void Get(TextColumnData* col, int32_t idx, String* dest);

// Create the column index
std::unordered_map<int8_t, std::vector<int32_t>>* CreateInt8Index();
std::unordered_map<int16_t, std::vector<int32_t>>* CreateInt16Index();
std::unordered_map<int32_t, std::vector<int32_t>>* CreateInt32Index();
std::unordered_map<int64_t, std::vector<int32_t>>* CreateInt64Index();
std::unordered_map<double, std::vector<int32_t>>* CreateFloat64Index();
std::unordered_map<std::string, std::vector<int32_t>>* CreateStringIndex();

// Free the column index
void Free(std::unordered_map<int8_t, std::vector<int32_t>>* index);
void Free(std::unordered_map<int16_t, std::vector<int32_t>>* index);
void Free(std::unordered_map<int32_t, std::vector<int32_t>>* index);
void Free(std::unordered_map<int64_t, std::vector<int32_t>>* index);
void Free(std::unordered_map<double, std::vector<int32_t>>* index);
void Free(std::unordered_map<std::string, std::vector<int32_t>>* index);

// Insert tuple idx
void Insert(std::unordered_map<int8_t, std::vector<int32_t>>* index,
            int8_t value, int32_t tuple_idx);
void Insert(std::unordered_map<int16_t, std::vector<int32_t>>* index,
            int16_t value, int32_t tuple_idx);
void Insert(std::unordered_map<int32_t, std::vector<int32_t>>* index,
            int32_t value, int32_t tuple_idx);
void Insert(std::unordered_map<int64_t, std::vector<int32_t>>* index,
            int64_t value, int32_t tuple_idx);
void Insert(std::unordered_map<double, std::vector<int32_t>>* index,
            double value, int32_t tuple_idx);
void Insert(std::unordered_map<std::string, std::vector<int32_t>>* index,
            String* value, int32_t tuple_idx);

// Get the next tuple from index
int32_t GetNextTuple(std::unordered_map<int8_t, std::vector<int32_t>>* index,
                     int8_t value, int32_t prev_tuple, int32_t cardinality);
int32_t GetNextTuple(std::unordered_map<int16_t, std::vector<int32_t>>* index,
                     int16_t value, int32_t prev_tuple, int32_t cardinality);
int32_t GetNextTuple(std::unordered_map<int32_t, std::vector<int32_t>>* index,
                     int32_t value, int32_t prev_tuple, int32_t cardinality);
int32_t GetNextTuple(std::unordered_map<int64_t, std::vector<int32_t>>* index,
                     int64_t value, int32_t prev_tuple, int32_t cardinality);
int32_t GetNextTuple(std::unordered_map<double, std::vector<int32_t>>* index,
                     double value, int32_t prev_tuple, int32_t cardinality);
int32_t GetNextTuple(
    std::unordered_map<std::string, std::vector<int32_t>>* index, String* value,
    int32_t prev_tuple, int32_t cardinality);

}  // namespace kush::data