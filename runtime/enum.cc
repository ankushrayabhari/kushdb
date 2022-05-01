#include "runtime/enum.h"

#include <cstdint>
#include <deque>
#include <memory>

#include "absl/container/flat_hash_map.h"

#include "runtime/column_data.h"
#include "runtime/string.h"

namespace kush::runtime {

int32_t EnumManager::Register(std::string key_path, std::string map_path) {
  int32_t id = info_.size();
  auto& data = info_[id];
  ColumnData::OpenText(&data.keys, key_path.c_str());
  ColumnIndex::Open(&data.map, map_path.c_str());
  return id;
}

void EnumManager::GetKey(int32_t id, int32_t value, String::String* dest) {
  auto& data = info_.at(id);
  ColumnData::GetText(&data.keys, value, dest);
}

int32_t EnumManager::GetValue(int32_t id, std::string_view value) {
  auto& data = info_.at(id);
  String::String key{.data = value.data(), .length = (int32_t)value.size()};
  ColumnIndexBucket dest;
  ColumnIndex::GetText(&data.map, &key, &dest);
  return dest.data[0];
}

void GetKey(int32_t id, int32_t value, String::String* dest) {
  EnumManager::Get().GetKey(id, value, dest);
}

int32_t GetValue(int32_t id, std::string_view value) {
  return EnumManager::Get().GetValue(id, value);
}

}  // namespace kush::runtime