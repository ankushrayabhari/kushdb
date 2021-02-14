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

#include "data/column_data.h"

namespace kush::data {

// Helper functions to serialize a vector of data to disk in the correct column
// format.

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

template <>
void Serialize(std::string_view path,
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