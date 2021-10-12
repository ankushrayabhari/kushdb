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

namespace kush::runtime::ColumnIndex {

template struct ColumnIndexEntry<int8_t>;
template struct ColumnIndexEntry<int16_t>;
template struct ColumnIndexEntry<int32_t>;
template struct ColumnIndexEntry<int64_t>;
template struct ColumnIndexEntry<double>;

uint64_t hash(uint64_t k) {
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

void Open(ColumnIndex* col, const char* path) {
  int fd = open(path, O_RDONLY);
  if (fd == -1) {
    throw std::system_error(errno, std::generic_category());
  }

  struct stat sb;
  if (fstat(fd, &sb) == -1) {
    throw std::system_error(errno, std::generic_category());
  }
  col->file_length = static_cast<int32_t>(sb.st_size);

  col->data = reinterpret_cast<ColumnIndexData*>(
      mmap(nullptr, col->file_length, PROT_READ, MAP_PRIVATE, fd, 0));
  if (col->data == MAP_FAILED) {
    throw std::system_error(errno, std::generic_category());
  }

  close(fd);
}

template <typename T>
void GetImpl(ColumnIndex* col, T key, ColumnIndexBucket* dest) {
  uint64_t key_as_64;
  if constexpr (std::is_same_v<T, double>) {
    std::memcpy(&key_as_64, &key, sizeof(uint64_t));
  } else {
    key_as_64 = key;
  }

  uint64_t pos = col->data->array[hash(key_as_64) & col->data->mask];
  while (pos > 0) {
    auto entry = reinterpret_cast<ColumnIndexEntry<T>*>(
        reinterpret_cast<uint8_t*>(col->data) + pos);
    if (entry->key == key) {
      dest->data = entry->values;
      dest->size = entry->size;
      return;
    }
    pos = entry->next;
  }

  dest->data = nullptr;
  dest->size = 0;
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
      return;
    }
    pos = entry->next;
  }

  dest->data = nullptr;
  dest->size = 0;
  return;
}

void Close(ColumnIndex* col) {
  if (munmap(col->data, col->file_length) == -1) {
    throw std::system_error(errno, std::generic_category());
  }
}

template <typename T>
void Serialize(const char* path,
               std::unordered_map<T, std::vector<int32_t>>& index) {
  int fd = open(std::string(path).c_str(), O_RDWR | O_CREAT,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd == -1) {
    throw std::system_error(errno, std::generic_category());
  }

  // number of array entries in hash table should be >= 1.5 x entries
  uint64_t bits = 1;
  while ((1ull << bits) < (index.size() * 1.5)) bits++;

  // length = metadata struct size + number of array entries
  int32_t length =
      sizeof(ColumnIndexData) + ((1ull << bits) * sizeof(uint64_t));
  //           + (index entry + number of values for each value)
  for (const auto& [key, values] : index) {
    length += sizeof(ColumnIndexEntry<T>) + (sizeof(int32_t) * values.size());
  }

  if (posix_fallocate(fd, 0, length) != 0) {
    throw std::system_error(errno, std::generic_category());
  }

  auto data = reinterpret_cast<ColumnIndexData*>(
      mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
  if (data == MAP_FAILED) {
    throw std::system_error(errno, std::generic_category());
  }

  uint64_t mask = (1ull << bits) - 1;
  data->mask = mask;
  memset(data->array, 0, (1ull << bits) * sizeof(uint64_t));

  uint8_t* ptr = reinterpret_cast<uint8_t*>(data);
  uint64_t offset =
      sizeof(ColumnIndexData) + ((1ull << bits) * sizeof(uint64_t));
  for (const auto& [key, values] : index) {
    // write out the column index entry
    auto entry = reinterpret_cast<ColumnIndexEntry<T>*>(ptr + offset);
    entry->key = key;
    entry->size = values.size();
    memcpy(entry->values, values.data(), values.size() * sizeof(int32_t));

    // prepend to linked list of entries
    uint64_t pos = hash(key) & mask;
    entry->next = data->array[pos];
    data->array[pos] = offset;

    offset += sizeof(ColumnIndexEntry<T>) + values.size() * sizeof(int32_t);
  }
  assert(offset == length);

  if (munmap(data, length) != 0) {
    throw std::system_error(errno, std::generic_category());
  }

  if (close(fd) != 0) {
    throw std::system_error(errno, std::generic_category());
  }
}

template void Serialize(
    const char* path, std::unordered_map<int8_t, std::vector<int32_t>>& index);

template void Serialize(
    const char* path, std::unordered_map<int16_t, std::vector<int32_t>>& index);

template void Serialize(
    const char* path, std::unordered_map<int32_t, std::vector<int32_t>>& index);

template void Serialize(
    const char* path, std::unordered_map<int64_t, std::vector<int32_t>>& index);

template void Serialize(
    const char* path, std::unordered_map<double, std::vector<int32_t>>& index);

template <>
void Serialize(const char* path,
               std::unordered_map<std::string, std::vector<int32_t>>& index) {
  int fd = open(std::string(path).c_str(), O_RDWR | O_CREAT,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
  if (fd == -1) {
    throw std::system_error(errno, std::generic_category());
  }

  // number of array entries in hash table should be >= 1.5 x entries
  uint64_t bits = 1;
  while ((1ull << bits) < (index.size() * 1.5)) bits++;

  // length = metadata struct size + number of array entries
  int32_t length =
      sizeof(ColumnIndexData) + ((1ull << bits) * sizeof(uint64_t));
  //           + (index entry + number of values for each value)
  for (const auto& [key, values] : index) {
    length += sizeof(ColumnIndexEntry<std::string>) +
              (sizeof(int32_t) * values.size());
  }
  //           + (key.size() + 1 for each key)
  uint64_t string_data_base = length;
  for (const auto& [key, values] : index) {
    length += key.size() + 1;
  }

  if (posix_fallocate(fd, 0, length) != 0) {
    throw std::system_error(errno, std::generic_category());
  }

  auto data = reinterpret_cast<ColumnIndexData*>(
      mmap(nullptr, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
  if (data == MAP_FAILED) {
    throw std::system_error(errno, std::generic_category());
  }

  uint64_t mask = (1ull << bits) - 1;
  data->mask = mask;
  memset(data->array, 0, (1ull << bits) * sizeof(uint64_t));

  uint8_t* ptr = reinterpret_cast<uint8_t*>(data);
  uint64_t offset =
      sizeof(ColumnIndexData) + ((1ull << bits) * sizeof(uint64_t));
  uint64_t string_data_offset = string_data_base;
  std::hash<std::string> hasher;
  for (const auto& [key, values] : index) {
    // write out the column index entry
    auto entry = reinterpret_cast<ColumnIndexEntry<std::string>*>(ptr + offset);
    entry->str_len = key.size();
    entry->str_offset = string_data_offset;
    entry->size = values.size();
    memcpy(entry->values, values.data(), values.size() * sizeof(int32_t));
    // write out string data
    auto string_entry = reinterpret_cast<char*>(ptr + string_data_offset);
    memcpy(string_entry, key.c_str(), key.size() + 1);

    // prepend to linked list of entries
    uint64_t pos = hasher(key) & mask;
    entry->next = data->array[pos];
    data->array[pos] = offset;

    offset +=
        sizeof(ColumnIndexEntry<std::string>) + values.size() * sizeof(int32_t);
    string_data_offset += key.size() + 1;
  }
  assert(offset == string_data_base);
  assert(string_data_offset == length);

  if (munmap(data, length) != 0) {
    throw std::system_error(errno, std::generic_category());
  }

  if (close(fd) != 0) {
    throw std::system_error(errno, std::generic_category());
  }
}

}  // namespace kush::runtime::ColumnIndex