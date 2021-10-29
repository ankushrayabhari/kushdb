#pragma once

#include <cstdint>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "absl/time/civil_time.h"
#include "absl/time/time.h"

#include "runtime/column_data.h"
#include "runtime/column_index.h"

#define DECLARE_NULL_COL(T, x)  \
  std::vector<T> x;             \
  std::vector<int8_t> x##_null; \
  std::unordered_map<T, std::vector<int32_t>> x##_index;

#define DECLARE_NOT_NULL_COL(T, x) \
  std::vector<T> x;                \
  std::unordered_map<T, std::vector<int32_t>> x##_index;

#define APPEND_NULL(x, parse, data, tuple_idx) \
  x.push_back(parse(data));                    \
  x##_null.push_back(data.empty());            \
  if (!x##_null.back()) x##_index[x.back()].push_back(tuple_idx);

#define APPEND_NOT_NULL(x, parse, data, tuple_idx) \
  x.push_back(parse(data));                        \
  x##_index[x.back()].push_back(tuple_idx);

#define SERIALIZE_NULL(T, id, dest, file)                              \
  kush::runtime::ColumnData::Serialize<T>(                             \
      std::string(dest) + std::string(file) + ".kdb", id);             \
  kush::runtime::ColumnData::Serialize<int8_t>(                        \
      std::string(dest) + std::string(file) + "_null.kdb", id##_null); \
  kush::runtime::ColumnIndex::Serialize<T>(                            \
      std::string(dest) + std::string(file) + ".kdbindex", id##_index);

#define SERIALIZE_NOT_NULL(T, id, dest, file)              \
  kush::runtime::ColumnData::Serialize<T>(                 \
      std::string(dest) + std::string(file) + ".kdb", id); \
  kush::runtime::ColumnIndex::Serialize<T>(                \
      std::string(dest) + std::string(file) + ".kdbindex", id##_index);

namespace kush::util {

bool keepgoing(const std::string& s) {
  if (s.size() < 2) {
    return true;
  }

  if (s.size() == 2) {
    return s.back() != '"';
  }

  if (s.back() == '"') {
    return s[s.size() - 2] == '\\';
  }

  return true;
}

std::vector<std::string> Split(const std::string& s, char delim, int num_cols) {
  std::stringstream ss(s);
  std::vector<std::string> elems;
  for (int i = 0; i < num_cols; i++) {
    std::string to_add;
    std::getline(ss, to_add, delim);

    if (to_add.empty() || to_add[0] != '"') {
      elems.push_back(to_add);
      continue;
    }

    while (ss.good() && keepgoing(to_add)) {
      std::string item;
      std::getline(ss, item, delim);

      to_add.push_back(delim);
      to_add += item;
    }

    elems.push_back(to_add);
  }

  return elems;
}

int64_t ParseDate(const std::string& s) {
  if (s.empty()) {
    return 0;
  }

  auto parts = Split(s, '-', 3);
  auto day = absl::CivilDay(std::stoi(parts[0]), std::stoi(parts[1]),
                            std::stoi(parts[2]));
  absl::TimeZone utc = absl::UTCTimeZone();
  absl::Time time = absl::FromCivil(day, utc);
  return absl::ToUnixMillis(time);
}

int8_t ParseInt8(const std::string& s) {
  if (s.empty()) {
    return 0;
  }

  return (int8_t)std::stoi(s);
}

int16_t ParseInt16(const std::string& s) {
  if (s.empty()) {
    return 0;
  }

  return (int16_t)std::stoi(s);
}

int32_t ParseInt32(const std::string& s) {
  if (s.empty()) {
    return 0;
  }

  return std::stoi(s);
}

int64_t ParseInt64(const std::string& s) {
  if (s.empty()) {
    return 0;
  }

  return std::stol(s);
}

double ParseDouble(const std::string& s) {
  if (s.empty()) {
    return 0;
  }

  return std::stod(s);
}

int8_t ParseBool(const std::string& s) {
  if (s.empty()) {
    return 0;
  }

  return s == "true" ? 1 : 0;
}

std::string ParseString(const std::string& s) {
  if (s.empty()) return s;

  if (s.size() >= 2 && s.front() == '\"' && s.back() == '\"') {
    return s.substr(1, s.size() - 2);
  } else {
    return s;
  }
}

std::mutex cout_mutex;
void Print(std::string_view x) {
  const std::lock_guard<std::mutex> lock(cout_mutex);
  std::cout << x << std::endl;
}

}  // namespace kush::util
