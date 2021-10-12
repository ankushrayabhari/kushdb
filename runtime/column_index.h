#pragma once

#include <cstdint>
#include <cstring>
#include <exception>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "runtime/column_index_bucket.h"
#include "runtime/string.h"

namespace kush::runtime::ColumnIndex {

struct ColumnIndexData {
  uint64_t mask;
  uint64_t array[];
};

template <typename K>
struct ColumnIndexEntry {
  K key;
  uint64_t size;
  uint64_t next;
  int32_t values[];
};

template <>
struct ColumnIndexEntry<std::string> {
  uint64_t str_offset;
  uint64_t str_len;
  uint64_t size;
  uint64_t next;
  int32_t values[];
};

struct ColumnIndex {
  ColumnIndexData* data;
  int32_t file_length;
};

void Open(ColumnIndex* col, const char* path);

void GetInt8(ColumnIndex* col, int8_t key, ColumnIndexBucket* dest);
void GetInt16(ColumnIndex* col, int16_t key, ColumnIndexBucket* dest);
void GetInt32(ColumnIndex* col, int32_t key, ColumnIndexBucket* dest);
void GetInt64(ColumnIndex* col, int64_t key, ColumnIndexBucket* dest);
void GetFloat64(ColumnIndex* col, double key, ColumnIndexBucket* dest);
void GetText(ColumnIndex* col, String::String* key, ColumnIndexBucket* dest);

void Close(ColumnIndex* col);

template <typename T>
void Serialize(std::string_view path,
               std::unordered_map<T, std::vector<int32_t>>& index);

}  // namespace kush::runtime::ColumnIndex