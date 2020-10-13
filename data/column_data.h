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

namespace skinner {

/*
 * Hybrid in-memory/disk based column data based on
 * https://github.com/TimoKersten/db-engine-paradigms/blob/master/include/common/runtime/Mmap.hpp
 */
template <class T>
class ColumnData {
  uint32_t cardinality;
  uint32_t capacity;
  T* data_;
  bool persistent;
  uint32_t file_length;

 public:
  // In-memory, mutable column
  ColumnData()
      : cardinality(0),
        capacity(0),
        data_(nullptr),
        persistent(false),
        file_length(0) {}

  // File-backed, immutable column
  ColumnData(const std::string& path)
      : persistent(true), cardinality(0), capacity(0) {
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
      if (persistent) {
        assert(munmap(data_, file_length) == 0);
      } else {
        delete data_;
      }
    }
  }

  void serialize(const std::string& path) {
    int fd = open(path.c_str(), O_RDWR | O_CREAT,
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    assert(fd != -1);

    int length;
    if (persistent) {
      length = file_length;
    } else {
      length = sizeof(T) * cardinality;
    }

    assert(posix_fallocate(fd, 0, length) == 0);
    T* data = reinterpret_cast<T*>(
        mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    assert(data != MAP_FAILED);
    memcpy(data, data_, length);
    assert(munmap(data, length) == 0);
    assert(close(fd) == 0);
  }

  uint32_t size() const {
    if (persistent) {
      return file_length / sizeof(T);
    }
    return cardinality;
  }

  T* begin() const { return data_; }

  T* end() const { return data_ + size(); }

  void reset(uint32_t n) {
    if (persistent) {
      throw std::runtime_error("Can't reset persistent Vector");
    }

    if (data_) {
      delete data_;
    }
    data_ = new T[n];
    cardinality = 0;
    capacity = n;
  }

  void push_back(const T& el) {
    assert(!persistent);
    assert(cardinality < capacity);
    data_[cardinality++] = el;
  }

  T& operator[](uint32_t idx) { return data_[idx]; }

  const T& operator[](uint32_t idx) const { return data_[idx]; }
};

}  // namespace skinner