#include "runtime/column_data.h"

#include <cstdint>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>
#include <vector>

namespace kush::runtime::ColumnData {

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

void OpenInt8(Int8ColumnData* col, const char* path) {
  OpenImpl(col->data, col->file_length, path);
}

void OpenInt16(Int16ColumnData* col, const char* path) {
  OpenImpl(col->data, col->file_length, path);
}

void OpenInt32(Int32ColumnData* col, const char* path) {
  OpenImpl(col->data, col->file_length, path);
}

void OpenInt64(Int64ColumnData* col, const char* path) {
  OpenImpl(col->data, col->file_length, path);
}

void OpenFloat64(Float64ColumnData* col, const char* path) {
  OpenImpl(col->data, col->file_length, path);
}

void OpenText(TextColumnData* col, const char* path) {
  OpenImpl(col->data, col->file_length, path);
}

// ------ Close --------

template <typename T>
inline void CloseImpl(T* column) {
  if (column->data) {
    munmap(column->data, column->file_length);
  }
}

void CloseInt8(Int8ColumnData* col) { CloseImpl(col); }

void CloseInt16(Int16ColumnData* col) { CloseImpl(col); }

void CloseInt32(Int32ColumnData* col) { CloseImpl(col); }

void CloseInt64(Int64ColumnData* col) { CloseImpl(col); }

void CloseFloat64(Float64ColumnData* col) { CloseImpl(col); }

void CloseText(TextColumnData* col) { CloseImpl(col); }

// ------ Size --------

int32_t SizeInt8(Int8ColumnData* col) {
  return col->file_length / sizeof(int16_t);
}

int32_t SizeInt16(Int16ColumnData* col) {
  return col->file_length / sizeof(int16_t);
}

int32_t SizeInt32(Int32ColumnData* col) {
  return col->file_length / sizeof(int32_t);
}

int32_t SizeInt64(Int64ColumnData* col) {
  return col->file_length / sizeof(int64_t);
}

int32_t SizeFloat64(Float64ColumnData* col) {
  return col->file_length / sizeof(double);
}

int32_t SizeText(TextColumnData* col) { return col->data->cardinality; }

// ------ Get --------

int8_t GetInt8(Int8ColumnData* col, int32_t idx) { return col->data[idx]; }

int16_t GetInt16(Int16ColumnData* col, int32_t idx) { return col->data[idx]; }

int32_t GetInt32(Int32ColumnData* col, int32_t idx) { return col->data[idx]; }

int64_t GetInt64(Int64ColumnData* col, int32_t idx) { return col->data[idx]; }

double GetFloat64(Float64ColumnData* col, int32_t idx) {
  return col->data[idx];
}

void GetText(TextColumnData* col, int32_t idx, String::String* dest) {
  const auto& slot = col->data->slot[idx];
  dest->data = reinterpret_cast<const char*>(col->data) + slot.offset;
  dest->length = slot.length;
}

}  // namespace kush::runtime::ColumnData