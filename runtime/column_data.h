#pragma once

#include <cstdint>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "runtime/string.h"

namespace kush::runtime::ColumnData {

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
void OpenInt8(Int8ColumnData* col, const char* path);
void OpenInt16(Int16ColumnData* col, const char* path);
void OpenInt32(Int32ColumnData* col, const char* path);
void OpenInt64(Int64ColumnData* col, const char* path);
void OpenFloat64(Float64ColumnData* col, const char* path);
void OpenText(TextColumnData* col, const char* path);

// Close column data file
void CloseInt8(Int8ColumnData* col);
void CloseInt16(Int16ColumnData* col);
void CloseInt32(Int32ColumnData* col);
void CloseInt64(Int64ColumnData* col);
void CloseFloat64(Float64ColumnData* col);
void CloseText(TextColumnData* col);

// Size implementations
int32_t SizeInt8(Int8ColumnData* col);
int32_t SizeInt16(Int16ColumnData* col);
int32_t SizeInt32(Int32ColumnData* col);
int32_t SizeInt64(Int64ColumnData* col);
int32_t SizeFloat64(Float64ColumnData* col);
int32_t SizeText(TextColumnData* col);

// Get the ith element of the column
int8_t GetInt8(Int8ColumnData* col, int32_t idx);
int16_t GetInt16(Int16ColumnData* col, int32_t idx);
int32_t GetInt32(Int32ColumnData* col, int32_t idx);
int64_t GetInt64(Int64ColumnData* col, int32_t idx);
double GetFloat64(Float64ColumnData* col, int32_t idx);
void GetText(TextColumnData* col, int32_t idx, String::String* dest);

template <typename T>
extern void Serialize(std::string_view path, const std::vector<T>& contents);

}  // namespace kush::runtime::ColumnData