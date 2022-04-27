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

#include "runtime/file_manager.h"

namespace kush::runtime::ColumnData {

// ------ Open --------

template <typename T>
inline void OpenImpl(T*& data, uint64_t& file_length, const char* path) {
  auto fi = FileManager::Get().Open(path);
  file_length = fi.file_length;
  data = reinterpret_cast<T*>(fi.data);
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
  // TODO: update buffer pool manager to close
}

void CloseInt8(Int8ColumnData* col) { CloseImpl(col); }

void CloseInt16(Int16ColumnData* col) { CloseImpl(col); }

void CloseInt32(Int32ColumnData* col) { CloseImpl(col); }

void CloseInt64(Int64ColumnData* col) { CloseImpl(col); }

void CloseFloat64(Float64ColumnData* col) { CloseImpl(col); }

void CloseText(TextColumnData* col) { CloseImpl(col); }

// ------ Size --------

int32_t SizeInt8(Int8ColumnData* col) {
  return col->file_length / sizeof(int8_t);
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

// ------ Serialize --------

template <typename T>
void Serialize(std::string_view path, const std::vector<T>& contents) {
  int fd = open(std::string(path).c_str(), O_RDWR | O_CREAT,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd == -1) {
    throw std::system_error(
        errno, std::generic_category(),
        std::string(__FILE__) + ":" + std::to_string(__LINE__));
  }

  uint64_t length = sizeof(T) * contents.size();

  if (posix_fallocate(fd, 0, length) != 0) {
    throw std::runtime_error(
        std::string(__FILE__) + ":" + std::to_string(__LINE__) +
        " fallocate failed with length " + std::to_string(length));
  }

  T* data = reinterpret_cast<T*>(
      mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
  if (data == MAP_FAILED) {
    throw std::system_error(
        errno, std::generic_category(),
        std::string(__FILE__) + ":" + std::to_string(__LINE__));
  }
  memcpy(data, contents.data(), length);

  if (munmap(data, length) != 0) {
    throw std::system_error(
        errno, std::generic_category(),
        std::string(__FILE__) + ":" + std::to_string(__LINE__));
  }

  if (close(fd) != 0) {
    throw std::system_error(
        errno, std::generic_category(),
        std::string(__FILE__) + ":" + std::to_string(__LINE__));
  }
}

template void Serialize(std::string_view path,
                        const std::vector<int8_t>& contents);

template void Serialize(std::string_view path,
                        const std::vector<int16_t>& contents);

template void Serialize(std::string_view path,
                        const std::vector<int32_t>& contents);

template void Serialize(std::string_view path,
                        const std::vector<int64_t>& contents);

template void Serialize(std::string_view path,
                        const std::vector<double>& contents);

template <>
void Serialize(std::string_view path,
               const std::vector<std::string>& contents) {
  int fd = open(std::string(path).c_str(), O_RDWR | O_CREAT,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd == -1) {
    throw std::system_error(
        errno, std::generic_category(),
        std::string(__FILE__) + ":" + std::to_string(__LINE__));
  }

  // 8 bytes for the metadata cardinality + sizeof(StringEntry) bytes per slot
  uint64_t length = 8 + sizeof(StringEntry) * contents.size();

  // 1 byte per character + 1 null terminator
  for (auto s : contents) length += s.size() + 1;

  if (posix_fallocate(fd, 0, length) != 0) {
    throw std::runtime_error(
        std::string(__FILE__) + ":" + std::to_string(__LINE__) +
        " fallocate failed with length " + std::to_string(length));
  }

  auto data = reinterpret_cast<runtime::ColumnData::StringMetadata*>(
      mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
  if (data == MAP_FAILED) {
    throw std::system_error(
        errno, std::generic_category(),
        std::string(__FILE__) + ":" + std::to_string(__LINE__));
  }

  data->cardinality = contents.size();
  uint64_t offset = 8 + sizeof(StringEntry) * contents.size();
  char* string_data_ = reinterpret_cast<char*>(data);

  for (int slot = 0; slot < contents.size(); slot++) {
    int32_t length = contents[slot].size();
    data->slot[slot].length = length;
    data->slot[slot].offset = offset;
    memcpy(string_data_ + offset, contents[slot].c_str(), length + 1);
    offset += length + 1;
  }

  if (munmap(data, length) != 0) {
    throw std::system_error(
        errno, std::generic_category(),
        std::string(__FILE__) + ":" + std::to_string(__LINE__));
  }

  if (close(fd) != 0) {
    throw std::system_error(
        errno, std::generic_category(),
        std::string(__FILE__) + ":" + std::to_string(__LINE__));
  }
}

}  // namespace kush::runtime::ColumnData