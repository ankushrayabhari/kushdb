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

void Open(Int8ColumnData* col, const char* path);
void Open(Int16ColumnData* col, const char* path);
void Open(Int32ColumnData* col, const char* path);
void Open(Int64ColumnData* col, const char* path);
void Open(Float64ColumnData* col, const char* path);
void Open(TextColumnData* col, const char* path);

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

}  // namespace kush::data