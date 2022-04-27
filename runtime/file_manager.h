#pragma once

#include <string>
#include <string_view>

#include "absl/container/flat_hash_map.h"

namespace kush::runtime {

struct FileInformation {
  void* data;
  uint64_t file_length;
};

class FileManager {
 public:
  static FileManager& Get() {
    static FileManager instance;  // Guaranteed to be destroyed.
                                  // Instantiated on first use.
    return instance;
  }

 private:
  FileManager() = default;
  ~FileManager();

 public:
  FileManager(FileManager const&) = delete;
  void operator=(FileManager const&) = delete;

  FileInformation Open(std::string_view path);

 private:
  absl::flat_hash_map<std::string, FileInformation> info_;
};

}  // namespace kush::runtime
