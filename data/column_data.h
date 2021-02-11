#pragma once

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <cassert>
#include <cstdint>
#include <cstring>
#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace kush::data {

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

// Open the file
template <typename T>
inline void OpenImpl(T*& data, uint32_t& file_length, const char* path) {
  int fd = open(path, O_RDONLY);
  assert(fd != -1);

  struct stat sb;
  assert(fstat(fd, &sb) != -1);
  file_length = static_cast<uint32_t>(sb.st_size);

  data = reinterpret_cast<T*>(
      mmap(nullptr, file_length, PROT_READ, MAP_PRIVATE, fd, 0));
  assert(data != MAP_FAILED);
  assert(close(fd) == 0);
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

// Close the file
template <typename T>
inline void CloseImpl(T* column) {
  if (column->data) {
    assert(munmap(column->data, column->file_length) == 0);
  }
}

void Close(Int16ColumnData* col) { CloseImpl(col); }

void Close(Int32ColumnData* col) { CloseImpl(col); }

void Close(Int64ColumnData* col) { CloseImpl(col); }

void Close(Float64ColumnData* col) { CloseImpl(col); }

void Close(TextColumnData* col) { CloseImpl(col); }

// Size implementations
uint32_t Size(Int16ColumnData* col) {
  return col->file_length / sizeof(int16_t);
}

uint32_t Size(Int32ColumnData* col) {
  return col->file_length / sizeof(int32_t);
}

uint32_t Size(Int64ColumnData* col) {
  return col->file_length / sizeof(int64_t);
}

uint32_t Size(Float64ColumnData* col) {
  return col->file_length / sizeof(double);
}

uint32_t Size(TextColumnData* col) { return col->data->cardinality; }

// Get the ith element of the column
int16_t Get(Int16ColumnData* col, uint32_t idx) { return col->data[idx]; }

int32_t Get(Int32ColumnData* col, uint32_t idx) { return col->data[idx]; }

int64_t Get(Int64ColumnData* col, uint32_t idx) { return col->data[idx]; }

double Get(Float64ColumnData* col, uint32_t idx) { return col->data[idx]; }

std::string_view Get(TextColumnData* col, uint32_t idx) {
  const auto& slot = col->data->slot[idx];
  return std::string_view(
      reinterpret_cast<const char*>(col->data) + slot.offset, slot.length);
}

// Functions to serialize a vector of data to disk in the correct column format.
template <typename T>
void Serialize(std::string_view path, const std::vector<T>& contents) {
  int fd = open(std::string(path).c_str(), O_RDWR | O_CREAT,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  assert(fd != -1);

  int length = sizeof(T) * contents.size();

  assert(posix_fallocate(fd, 0, length) == 0);
  T* data = reinterpret_cast<T*>(
      mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
  assert(data != MAP_FAILED);
  memcpy(data, contents.data(), length);
  assert(munmap(data, length) == 0);
  assert(close(fd) == 0);
}

static void Serialize(std::string_view path,
                      const std::vector<std::string>& contents) {
  int fd = open(std::string(path).c_str(), O_RDWR | O_CREAT,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  assert(fd != -1);

  // 4 bytes for the metadata cardinality + (4 + 4) bytes per slot
  uint32_t length = 4 + 8 * contents.size();

  // 1 byte per character + 1 null terminator
  for (auto s : contents) length += s.size() + 1;

  assert(posix_fallocate(fd, 0, length) == 0);

  auto data = reinterpret_cast<StringMetadata*>(
      mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
  assert(data != MAP_FAILED);

  data->cardinality = contents.size();
  uint32_t offset = 4 + 8 * contents.size();
  char* string_data_ = reinterpret_cast<char*>(data);

  for (int slot = 0; slot < contents.size(); slot++) {
    uint32_t length = contents[slot].size();
    data->slot[slot].length = length;
    data->slot[slot].offset = offset;
    memcpy(string_data_ + offset, contents[slot].c_str(), length + 1);
    offset += length + 1;
  }

  assert(munmap(data, length) == 0);
  assert(close(fd) == 0);
}

}  // namespace kush::data