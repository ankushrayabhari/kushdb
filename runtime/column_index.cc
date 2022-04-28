#include "runtime/column_index.h"

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

#include "runtime/file_manager.h"

namespace kush::runtime::ColumnIndex {

template struct ColumnIndexEntry<int8_t>;
template struct ColumnIndexEntry<int16_t>;
template struct ColumnIndexEntry<int32_t>;
template struct ColumnIndexEntry<int64_t>;
template struct ColumnIndexEntry<double>;

uint64_t Hash(uint64_t k) {
  const uint64_t m = 0xc6a4a7935bd1e995;
  const int r = 47;
  uint64_t h = 0x8445d61a4e774912 ^ (8 * m);
  k *= m;
  k ^= k >> r;
  k *= m;
  h ^= k;
  h *= m;
  h ^= h >> r;
  h *= m;
  h ^= h >> r;
  return h | (1ull << 63);
}

template <typename T>
uint64_t ToUInt64(T key) {
  uint64_t key_as_64;
  if constexpr (std::is_same_v<T, double>) {
    std::memcpy(&key_as_64, &key, sizeof(uint64_t));
  } else if constexpr (std::is_same_v<T, int8_t>) {
    key_as_64 = key;
    key_as_64 &= 0xFF;
  } else if constexpr (std::is_same_v<T, int16_t>) {
    key_as_64 = key;
    key_as_64 &= 0xFFFF;
  } else if constexpr (std::is_same_v<T, int32_t>) {
    key_as_64 = key;
    key_as_64 &= 0xFFFFFFFF;
  } else if constexpr (std::is_same_v<T, int32_t>) {
    key_as_64 = key;
    key_as_64 &= 0xFFFFFFFF;
  } else if constexpr (std::is_same_v<T, int64_t>) {
    key_as_64 = key;
  }
  return key_as_64;
}

void Open(ColumnIndex* col, const char* path) {
  auto fi = FileManager::Get().Open(path);
  col->data = reinterpret_cast<ColumnIndexData*>(fi.data);
  col->file_length = fi.file_length;
}

template <typename T>
void GetImpl(ColumnIndex* col, T key, ColumnIndexBucket* dest) {
  uint64_t key_as_64 = ToUInt64(key);
  uint64_t pos = col->data->array[Hash(key_as_64) & col->data->mask];
  while (pos > 0) {
    auto entry = reinterpret_cast<ColumnIndexEntry<T>*>(
        reinterpret_cast<uint8_t*>(col->data) + pos);
    if (entry->key == key) {
      dest->data = entry->values;
      dest->size = entry->size;
      dest->allocator = nullptr;
      dest->capacity = 0;
      return;
    }
    pos = entry->next;
  }

  dest->data = nullptr;
  dest->size = 0;
  dest->allocator = nullptr;
  dest->capacity = 0;
  return;
}

void GetInt8(ColumnIndex* col, int8_t key, ColumnIndexBucket* dest) {
  GetImpl(col, key, dest);
}

void GetInt16(ColumnIndex* col, int16_t key, ColumnIndexBucket* dest) {
  GetImpl(col, key, dest);
}

void GetInt32(ColumnIndex* col, int32_t key, ColumnIndexBucket* dest) {
  GetImpl(col, key, dest);
}

void GetInt64(ColumnIndex* col, int64_t key, ColumnIndexBucket* dest) {
  GetImpl(col, key, dest);
}

void GetFloat64(ColumnIndex* col, double key, ColumnIndexBucket* dest) {
  GetImpl(col, key, dest);
}

void GetText(ColumnIndex* col, String::String* key, ColumnIndexBucket* dest) {
  std::string_view key_as_sv(key->data, key->length);

  std::hash<std::string_view> hasher;
  uint64_t pos = col->data->array[hasher(key_as_sv) & col->data->mask];
  while (pos > 0) {
    auto entry = reinterpret_cast<ColumnIndexEntry<std::string>*>(
        reinterpret_cast<uint8_t*>(col->data) + pos);

    std::string_view entry_key_as_sv(
        reinterpret_cast<const char*>(col->data) + entry->str_offset,
        entry->str_len);
    if (entry_key_as_sv == key_as_sv) {
      dest->data = entry->values;
      dest->size = entry->size;
      dest->allocator = nullptr;
      dest->capacity = 0;
      return;
    }
    pos = entry->next;
  }

  dest->data = nullptr;
  dest->size = 0;
  dest->allocator = nullptr;
  dest->capacity = 0;
  return;
}

void Close(ColumnIndex* col) {
  // TODO: Update buffer pool manager to close
}

template <typename T>
void Serialize(std::string_view path,
               std::unordered_map<T, std::vector<int32_t>>& index) {
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
  uint64_t length =
      sizeof(ColumnIndexData) + ((1ull << bits) * sizeof(uint64_t));
  //           + (index entry + number of values for each value)
  for (const auto& [key, values] : index) {
    length += sizeof(ColumnIndexEntry<T>) + (sizeof(int32_t) * values.size());
  }

  if (posix_fallocate(fd, 0, length) != 0) {
    throw std::runtime_error(
        std::string(__FILE__) + ":" + std::to_string(__LINE__) +
        " fallocate failed with length " + std::to_string(length));
  }

  auto data = reinterpret_cast<ColumnIndexData*>(
      mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
  if (data == MAP_FAILED) {
    throw std::system_error(
        errno, std::generic_category(),
        std::string(__FILE__) + ":" + std::to_string(__LINE__));
  }

  uint64_t mask = (1ull << bits) - 1;
  data->mask = mask;
  memset(data->array, 0, (1ull << bits) * sizeof(uint64_t));

  uint8_t* ptr = reinterpret_cast<uint8_t*>(data);
  uint64_t offset =
      sizeof(ColumnIndexData) + ((1ull << bits) * sizeof(uint64_t));

  using iterator =
      typename std::unordered_map<T, std::vector<int32_t>>::iterator;
  std::unordered_map<uint64_t, std::vector<iterator>> pos_to_entries;
  for (auto it = index.begin(); it != index.end(); it++) {
    auto key = it->first;

    uint64_t key_as_64 = ToUInt64(key);
    uint64_t pos = Hash(key_as_64) & mask;
    pos_to_entries[pos].push_back(it);
  }

  for (const auto& [pos, entries] : pos_to_entries) {
    std::vector<uint64_t> offsets;
    for (const auto& index_entry : entries) {
      const auto& key = index_entry->first;
      const auto& values = index_entry->second;

      // write out the column index entry
      auto entry = reinterpret_cast<ColumnIndexEntry<T>*>(ptr + offset);
      entry->key = key;
      entry->size = values.size();
      memcpy(entry->values, values.data(), values.size() * sizeof(int32_t));

      offsets.push_back(offset);

      offset += sizeof(ColumnIndexEntry<T>) + values.size() * sizeof(int32_t);
    }

    // update linked list of offsets
    for (int i = offsets.size() - 1; i >= 0; i--) {
      auto offset = offsets[i];
      auto entry = reinterpret_cast<ColumnIndexEntry<T>*>(ptr + offset);

      entry->next = data->array[pos];
      data->array[pos] = offset;
    }
  }
  assert(offset == length);

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

template void Serialize(
    std::string_view path,
    std::unordered_map<int8_t, std::vector<int32_t>>& index);

template void Serialize(
    std::string_view path,
    std::unordered_map<int16_t, std::vector<int32_t>>& index);

template void Serialize(
    std::string_view path,
    std::unordered_map<int32_t, std::vector<int32_t>>& index);

template void Serialize(
    std::string_view path,
    std::unordered_map<int64_t, std::vector<int32_t>>& index);

template void Serialize(
    std::string_view path,
    std::unordered_map<double, std::vector<int32_t>>& index);

template <>
void Serialize(std::string_view path,
               std::unordered_map<std::string, std::vector<int32_t>>& index) {
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
  uint64_t length =
      sizeof(ColumnIndexData) + ((1ull << bits) * sizeof(uint64_t));
  //           + (index entry + number of values for each value)
  //           + (key.size() + 1 for each key)
  for (const auto& [key, values] : index) {
    length += sizeof(ColumnIndexEntry<std::string>) +
              (sizeof(int32_t) * values.size()) + key.size() + 1;
  }

  if (posix_fallocate(fd, 0, length) != 0) {
    throw std::runtime_error(
        std::string(__FILE__) + ":" + std::to_string(__LINE__) +
        " fallocate failed with length " + std::to_string(length));
  }

  auto data = reinterpret_cast<ColumnIndexData*>(
      mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
  if (data == MAP_FAILED) {
    throw std::system_error(
        errno, std::generic_category(),
        std::string(__FILE__) + ":" + std::to_string(__LINE__));
  }

  uint64_t mask = (1ull << bits) - 1;
  data->mask = mask;
  memset(data->array, 0, (1ull << bits) * sizeof(uint64_t));

  uint8_t* ptr = reinterpret_cast<uint8_t*>(data);
  uint64_t offset =
      sizeof(ColumnIndexData) + ((1ull << bits) * sizeof(uint64_t));
  std::hash<std::string> hasher;

  using iterator =
      typename std::unordered_map<std::string, std::vector<int32_t>>::iterator;
  std::unordered_map<uint64_t, std::vector<iterator>> pos_to_entries;
  for (auto it = index.begin(); it != index.end(); it++) {
    uint64_t pos = hasher(it->first) & mask;
    pos_to_entries[pos].push_back(it);
  }

  for (const auto& [pos, entries] : pos_to_entries) {
    std::vector<uint64_t> offsets;

    for (const auto& index_entry : entries) {
      const auto& key = index_entry->first;
      const auto& values = index_entry->second;

      auto entry_offset = offset;

      // write out the column index entry
      auto entry =
          reinterpret_cast<ColumnIndexEntry<std::string>*>(ptr + entry_offset);
      entry->str_len = key.size();
      entry->size = values.size();
      memcpy(entry->values, values.data(), values.size() * sizeof(int32_t));
      offset += sizeof(ColumnIndexEntry<std::string>) +
                values.size() * sizeof(int32_t);

      // write out string data
      entry->str_offset = offset;
      auto string_entry = reinterpret_cast<char*>(ptr + offset);
      memcpy(string_entry, key.c_str(), key.size() + 1);
      offset += key.size() + 1;

      offsets.push_back(entry_offset);
    }

    // update linked list of offsets
    for (int i = offsets.size() - 1; i >= 0; i--) {
      auto offset = offsets[i];
      auto entry =
          reinterpret_cast<ColumnIndexEntry<std::string>*>(ptr + offset);
      entry->next = data->array[pos];
      data->array[pos] = offset;
    }
  }
  assert(offset == length);

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

}  // namespace kush::runtime::ColumnIndex