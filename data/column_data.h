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

namespace kush {

/*
 * File-backed, immutable column based on
 * https://github.com/TimoKersten/db-engine-paradigms/blob/master/include/common/runtime/Mmap.hpp
 */
template <class T>
class ColumnData {
 public:
  ColumnData(const std::string& path) {
    int fd = open(path.c_str(), O_RDONLY);
    assert(fd != -1);

    struct stat sb;
    assert(fstat(fd, &sb) != -1);
    file_length = static_cast<uint64_t>(sb.st_size);

    data_ = reinterpret_cast<T*>(
        mmap(nullptr, file_length, PROT_READ, MAP_PRIVATE, fd, 0));
    assert(data_ != MAP_FAILED);
    assert(close(fd) == 0);
  }

  ColumnData(const ColumnData&) = delete;
  ColumnData& operator=(const ColumnData&) = delete;

  ~ColumnData() {
    if (data_) {
      assert(munmap(data_, file_length) == 0);
    }
  }

  static void Serialize(const std::string& path,
                        const std::vector<T>& contents) {
    int fd = open(path.c_str(), O_RDWR | O_CREAT,
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

  uint32_t Size() const { return file_length / sizeof(T); }

  const T& operator[](uint32_t idx) const { return data_[idx]; }

 private:
  T* data_;
  uint32_t file_length;
};

template <>
class ColumnData<std::string_view> {
 private:
  struct Metadata {
    uint32_t cardinality;
    struct {
      uint32_t length;
      uint32_t offset;
    } slot[];
  };

 public:
  ColumnData(const std::string& path) {
    int fd = open(path.c_str(), O_RDONLY);
    assert(fd != -1);

    struct stat sb;
    assert(fstat(fd, &sb) != -1);
    file_length = static_cast<uint64_t>(sb.st_size);

    data_ = reinterpret_cast<Metadata*>(
        mmap(nullptr, file_length, PROT_READ, MAP_PRIVATE, fd, 0));
    assert(data_ != MAP_FAILED);
    assert(close(fd) == 0);
  }

  ColumnData(const ColumnData&) = delete;
  ColumnData& operator=(const ColumnData&) = delete;

  ~ColumnData() {
    if (data_) {
      assert(munmap(data_, file_length) == 0);
    }
  }

  static void Serialize(const std::string& pathname,
                        const std::vector<std::string>& contents) {
    int fd = open(pathname.c_str(), O_RDWR | O_CREAT,
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    assert(fd != -1);

    // 4 bytes for the metadata cardinality + (4 + 4) bytes per slot
    uint32_t length = 4 + 8 * contents.size();

    // 1 byte per character + 1 null terminator
    for (auto s : contents) length += s.size() + 1;

    assert(posix_fallocate(fd, 0, length) == 0);

    auto data = reinterpret_cast<Metadata*>(
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

  uint32_t Size() const { return data_->cardinality; }

  std::string_view operator[](uint32_t idx) const {
    const auto& slot = data_->slot[idx];
    return std::string_view(reinterpret_cast<const char*>(data_) + slot.offset,
                            slot.length);
  }

 private:
  Metadata* data_;
  uint32_t file_length;
};

}  // namespace kush