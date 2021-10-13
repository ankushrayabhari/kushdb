#pragma once

#include <string>
#include <string_view>

#include "absl/container/flat_hash_map.h"

namespace kush::runtime {

struct FileInformation {
  void* data;
  uint64_t file_length;
};

class BufferPoolManager {
 public:
  static BufferPoolManager& Get() {
    static BufferPoolManager instance;  // Guaranteed to be destroyed.
                                        // Instantiated on first use.
    return instance;
  }

 private:
  BufferPoolManager() = default;
  ~BufferPoolManager();

 public:
  BufferPoolManager(BufferPoolManager const&) = delete;
  void operator=(BufferPoolManager const&) = delete;

  FileInformation Open(std::string_view path);

 private:
  absl::flat_hash_map<std::string, FileInformation> info_;
};

}  // namespace kush::runtime
