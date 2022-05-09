#include "runtime/file_manager.h"

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

FileInformation FileManager::Open(std::string_view path) {
  if (!info_.contains(path)) {
    int fd = open(std::string(path).c_str(), O_RDONLY);
    if (fd == -1) {
      std::cerr << path << std::endl;
      throw std::system_error(
          errno, std::generic_category(),
          std::string(__FILE__) + ":" + std::to_string(__LINE__));
    }

    struct stat sb;
    if (fstat(fd, &sb) == -1) {
      std::cerr << path << std::endl;
      throw std::system_error(
          errno, std::generic_category(),
          std::string(__FILE__) + ":" + std::to_string(__LINE__));
    }
    uint64_t file_length = sb.st_size;

    void* data = aligned_alloc(64, file_length);
    if (pread(fd, data, file_length, 0) < 0) {
      std::cerr << path << std::endl;
      throw std::system_error(
          errno, std::generic_category(),
          std::string(__FILE__) + ":" + std::to_string(__LINE__));
    }

    if (close(fd) != 0) {
      std::cerr << path << std::endl;
      throw std::system_error(
          errno, std::generic_category(),
          std::string(__FILE__) + ":" + std::to_string(__LINE__));
    }

    info_[path] = FileInformation{.data = data, .file_length = file_length};
  }

  return info_[path];
}

FileManager::~FileManager() {
  // TODO: clear files
}

}  // namespace kush::runtime
