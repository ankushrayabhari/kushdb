#include "runtime/buffer_pool_manager.h"

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

#include "absl/container/flat_hash_map.h"

namespace kush::runtime {

FileInformation BufferPoolManager::Open(std::string_view path) {
  if (!info_.contains(path)) {
    int fd = open(std::string(path).c_str(), O_RDONLY);
    if (fd == -1) {
      throw std::system_error(
          errno, std::generic_category(),
          std::string(__FILE__) + ":" + std::to_string(__LINE__));
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
      throw std::system_error(
          errno, std::generic_category(),
          std::string(__FILE__) + ":" + std::to_string(__LINE__));
    }
    uint64_t file_length = sb.st_size;

    void* data = mmap(nullptr, file_length, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
      throw std::system_error(
          errno, std::generic_category(),
          std::string(__FILE__) + ":" + std::to_string(__LINE__));
    }

    if (close(fd) != 0) {
      throw std::system_error(
          errno, std::generic_category(),
          std::string(__FILE__) + ":" + std::to_string(__LINE__));
    }

    info_[path] = FileInformation{.data = data, .file_length = file_length};
  }

  return info_[path];
}

BufferPoolManager::~BufferPoolManager() {
  // TODO: munmap files
}

}  // namespace kush::runtime
