#pragma once

#include <cstdint>
#include <deque>
#include <memory>

#include "absl/container/flat_hash_map.h"

#include "runtime/column_data.h"
#include "runtime/column_index.h"
#include "runtime/string.h"

namespace kush::runtime {

struct EnumData {
  ColumnData::TextColumnData keys;
  ColumnIndex::ColumnIndex map;
};

class EnumManager {
 public:
  static EnumManager& Get() {
    static EnumManager instance;
    return instance;
  }

 private:
  EnumManager() = default;
  ~EnumManager();

 public:
  EnumManager(EnumManager const&) = delete;
  void operator=(EnumManager const&) = delete;

  int32_t Register(std::string key_path, std::string map_path);
  void GetKey(int32_t id, int32_t value, String::String* dest);
  int32_t GetValue(int32_t id, std::string_view value);

 private:
  absl::flat_hash_map<int32_t, EnumData> info_;
};

void GetKey(int32_t id, int32_t value, String::String* dest);
int32_t GetValue(int32_t id, std::string_view value);

}  // namespace kush::runtime