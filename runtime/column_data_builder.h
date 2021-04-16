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
#include <string_view>
#include <system_error>
#include <vector>

#include "runtime/column_data.h"

namespace kush::data {

// Helper functions to serialize a vector of data to disk in the correct column
// format.

template <typename T>
void Serialize(std::string_view path, const std::vector<T>& contents) {
  int fd = open(std::string(path).c_str(), O_RDWR | O_CREAT,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd == -1) {
    throw std::system_error(errno, std::generic_category());
  }

  int length = sizeof(T) * contents.size();

  if (posix_fallocate(fd, 0, length) != 0) {
    throw std::system_error(errno, std::generic_category());
  }

  T* data = reinterpret_cast<T*>(
      mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
  if (data == MAP_FAILED) {
    throw std::system_error(errno, std::generic_category());
  }
  memcpy(data, contents.data(), length);

  if (munmap(data, length) != 0) {
    throw std::system_error(errno, std::generic_category());
  }

  if (close(fd) == 0) {
    throw std::system_error(errno, std::generic_category());
  }
}

template <>
void Serialize(std::string_view path,
               const std::vector<std::string>& contents) {
  int fd = open(std::string(path).c_str(), O_RDWR | O_CREAT,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd == -1) {
    throw std::system_error(errno, std::generic_category());
  }

  // 4 bytes for the metadata cardinality + (4 + 4) bytes per slot
  uint32_t length = 4 + 8 * contents.size();

  // 1 byte per character + 1 null terminator
  for (auto s : contents) length += s.size() + 1;

  if (posix_fallocate(fd, 0, length) != 0) {
    throw std::system_error(errno, std::generic_category());
  }

  auto data = reinterpret_cast<StringMetadata*>(
      mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
  if (data == MAP_FAILED) {
    throw std::system_error(errno, std::generic_category());
  }

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

  if (munmap(data, length) != 0) {
    throw std::system_error(errno, std::generic_category());
  }

  if (close(fd) == 0) {
    throw std::system_error(errno, std::generic_category());
  }
}

}  // namespace kush::data