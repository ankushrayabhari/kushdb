#include "runtime/enum.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <system_error>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "absl/container/flat_hash_map.h"

#include "runtime/file_manager.h"
#include "runtime/string.h"

namespace kush::runtime::Enum {

struct EnumData {
  uint64_t entry_offset;
  uint64_t mask;
  uint64_t array[];
};

struct EnumEntry {
  uint64_t str_offset;
  uint32_t str_len;
  uint32_t value;
};

void Serialize(std::string_view path,
               std::unordered_map<std::string, int32_t>& index) {
  int fd = open(std::string(path).c_str(), O_RDWR | O_CREAT,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd == -1) {
    throw std::system_error(
        errno, std::generic_category(),
        std::string(__FILE__) + ":" + std::to_string(__LINE__));
  }

  // number of array entries in hash table should be >= 1.5 x entries
  uint64_t bits = 1;
  while ((1ull << bits) < (index.size() * 1.5)) bits++;

  // length = metadata struct size + number of array entries
  uint64_t length = sizeof(EnumData) + ((1ull << bits) * sizeof(uint64_t));
  //           + index entry + (key.size() + 1 for each key)
  for (const auto& [key, values] : index) {
    length += sizeof(EnumEntry) + key.size() + 1;
  }

  if (posix_fallocate(fd, 0, length) != 0) {
    throw std::runtime_error(
        std::string(__FILE__) + ":" + std::to_string(__LINE__) +
        " fallocate failed with length " + std::to_string(length));
  }

  auto data = reinterpret_cast<EnumData*>(
      mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
  if (data == MAP_FAILED) {
    throw std::system_error(
        errno, std::generic_category(),
        std::string(__FILE__) + ":" + std::to_string(__LINE__));
  }
  uint8_t* ptr = reinterpret_cast<uint8_t*>(data);

  // first compute key order
  std::vector<std::unordered_map<std::string, int32_t>::iterator> keys_in_order(
      index.size(), index.end());
  for (auto it = index.begin(); it != index.end(); it++) {
    keys_in_order[it->second] = it;
  }

  data->entry_offset = sizeof(EnumData) + ((1ull << bits) * sizeof(uint64_t));
  auto enum_array_ptr = reinterpret_cast<EnumEntry*>(ptr + data->entry_offset);

  uint64_t string_offset = sizeof(EnumData) +
                           ((1ull << bits) * sizeof(uint64_t)) +
                           index.size() * sizeof(EnumEntry);

  // write out all the enum entries first
  for (auto it : keys_in_order) {
    auto key = it->first;
    auto id = it->second;

    auto entry = &enum_array_ptr[id];
    entry->str_offset = string_offset;
    entry->str_len = key.size();
    entry->value = id;

    // write out the string
    auto string_entry = reinterpret_cast<char*>(ptr + string_offset);
    memcpy(string_entry, key.c_str(), key.size() + 1);
    string_offset += key.size() + 1;
  }
  assert(string_offset == length);

  // insert into the hash table
  uint64_t mask = (1ull << bits) - 1;
  data->mask = mask;
  memset(data->array, 0, (1ull << bits) * sizeof(uint64_t));

  std::hash<std::string> hasher;
  for (const auto& [key, value] : index) {
    uint64_t pos = hasher(key) & mask;
    while (data->array[pos] != 0) {
      pos = (pos + 1) & mask;
    }
    data->array[pos] = sizeof(EnumData) + ((1ull << bits) * sizeof(uint64_t)) +
                       value * sizeof(EnumEntry);
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

void EnumManager::GetKey(int32_t id, int32_t value, String::String* dest) {
  if (!file_.contains(id)) {
    file_[id] = FileManager::Get().Open(info_.at(id)).data;
  }
  auto data = reinterpret_cast<uint8_t*>(file_.at(id));
  auto enum_data = reinterpret_cast<EnumData*>(data);
  auto enum_array_ptr =
      reinterpret_cast<EnumEntry*>(data + enum_data->entry_offset);
  dest->length = enum_array_ptr[value].str_len;
  dest->data = reinterpret_cast<char*>(data + enum_array_ptr[value].str_offset);
}

int32_t EnumManager::GetValue(int32_t id, std::string value) {
  if (!file_.contains(id)) {
    file_[id] = FileManager::Get().Open(info_.at(id)).data;
  }
  auto data = reinterpret_cast<uint8_t*>(file_.at(id));
  auto enum_data = reinterpret_cast<EnumData*>(data);

  std::hash<std::string> hasher;
  uint64_t pos = hasher(value) & enum_data->mask;
  while (enum_data->array[pos] != 0) {
    auto entry = reinterpret_cast<EnumEntry*>(data + enum_data->array[pos]);

    std::string_view value_as_sv(
        reinterpret_cast<const char*>(data + entry->str_offset),
        entry->str_len);

    if (value_as_sv == value) {
      return entry->value;
    }

    pos = (pos + 1) & enum_data->mask;
  }
  return -1;
}

int32_t EnumManager::Register(std::string_view enum_path) {
  int32_t id = info_.size();
  info_[id] = enum_path;
  return id;
}

void GetKey(int32_t id, int32_t value, String::String* dest) {
  EnumManager::Get().GetKey(id, value, dest);
}

int32_t GetValue(int32_t id, std::string_view value) {
  return EnumManager::Get().GetValue(id, std::string(value));
}

}  // namespace kush::runtime::Enum