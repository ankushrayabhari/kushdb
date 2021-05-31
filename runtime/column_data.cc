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

void Get(TextColumnData* col, int32_t idx, String::String* dest) {
  const auto& slot = col->data->slot[idx];
  dest->data = reinterpret_cast<const char*>(col->data) + slot.offset;
  dest->length = slot.length;
}

}  // namespace kush::runtime::ColumnData