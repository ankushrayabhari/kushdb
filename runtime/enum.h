#pragma once

#include <cstdint>
#include <deque>
#include <memory>

#include "absl/container/flat_hash_map.h"

#include "runtime/file_manager.h"
#include "runtime/string.h"

namespace kush::runtime::Enum {

void Serialize(std::string_view path,
               std::unordered_map<std::string, int32_t>& index);

class EnumManager {
 public:
  static EnumManager& Get() {
    static EnumManager instance;
    return instance;
  }

 private:
  EnumManager() = default;
  ~EnumManager() = default;

 public:
  EnumManager(EnumManager const&) = delete;
  void operator=(EnumManager const&) = delete;

  int32_t Register(std::string_view path);

  void GetKey(int32_t id, int32_t value, String::String* dest);
  int32_t GetValue(int32_t id, std::string value);

 private:
  absl::flat_hash_map<int32_t, std::string> info_;
  absl::flat_hash_map<int32_t, void*> file_;
};

void GetKey(int32_t id, int32_t value, String::String* dest);
int32_t GetValue(int32_t id, std::string_view value);

}  // namespace kush::runtime::Enum