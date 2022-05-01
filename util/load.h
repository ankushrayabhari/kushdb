#pragma once

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "runtime/column_data.h"
#include "runtime/column_index.h"
#include "runtime/date.h"
#include "runtime/enum.h"

#define DECLARE_NULL_COL(T, x)  \
  std::vector<T> x;             \
  std::vector<int8_t> x##_null; \
  std::unordered_map<T, std::vector<int32_t>> x##_index;

#define DECLARE_NOT_NULL_COL(T, x) \
  std::vector<T> x;                \
  std::unordered_map<T, std::vector<int32_t>> x##_index;

#define DECLARE_NULL_ENUM_COL(x)                               \
  std::vector<int> x;                                          \
  std::vector<int8_t> x##_null;                                \
  std::unordered_map<int32_t, std::vector<int32_t>> x##_index; \
  std::unordered_map<std::string, int32_t> x##_dictionary;

#define DECLARE_NOT_NULL_ENUM_COL(x)                           \
  std::vector<int> x;                                          \
  std::unordered_map<int32_t, std::vector<int32_t>> x##_index; \
  std::unordered_map<std::string, int32_t> x##_dictionary;

#define APPEND_NULL_ENUM(x, data, tuple_idx)                       \
  auto x##_parsed = ParseString(data);                             \
  x##_null.push_back(x##_parsed.empty());                          \
  if (x##_parsed.empty()) {                                        \
    x.push_back(-1);                                               \
  } else {                                                         \
    if (x##_dictionary.find(x##_parsed) == x##_dictionary.end()) { \
      auto id = x##_dictionary.size();                             \
      x##_dictionary[x##_parsed] = id;                             \
    }                                                              \
    x.push_back(x##_dictionary.at(x##_parsed));                    \
    x##_index[x.back()].push_back(tuple_idx);                      \
  }

#define APPEND_NOT_NULL_ENUM(x, data, tuple_idx)                 \
  auto x##_parsed = ParseString(data);                           \
  if (x##_dictionary.find(x##_parsed) == x##_dictionary.end()) { \
    auto id = x##_dictionary.size();                             \
    x##_dictionary[x##_parsed] = id;                             \
  }                                                              \
  x.push_back(x##_dictionary.at(x##_parsed));                    \
  x##_index[x.back()].push_back(tuple_idx);

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

#define SERIALIZE_NULL_ENUM(id, dest, file)                             \
  kush::runtime::ColumnData::Serialize<int32_t>(                        \
      std::string(dest) + std::string(file) + ".kdb", id);              \
  kush::runtime::ColumnData::Serialize<int8_t>(                         \
      std::string(dest) + std::string(file) + "_null.kdb", id##_null);  \
  kush::runtime::ColumnIndex::Serialize<int32_t>(                       \
      std::string(dest) + std::string(file) + ".kdbindex", id##_index); \
  kush::runtime::Enum::Serialize(                                       \
      std::string(dest) + std::string(file) + ".kdbenum", id##_dictionary);

#define SERIALIZE_NOT_NULL_ENUM(id, dest, file)                         \
  kush::runtime::ColumnData::Serialize<int32_t>(                        \
      std::string(dest) + std::string(file) + ".kdb", id);              \
  kush::runtime::ColumnIndex::Serialize<int32_t>(                       \
      std::string(dest) + std::string(file) + ".kdbindex", id##_index); \
  kush::runtime::Enum::Serialize(                                       \
      std::string(dest) + std::string(file) + ".kdbenum", id##_dictionary);

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

int32_t ParseDate(const std::string& s) {
  if (s.empty()) {
    return runtime::Date::DateBuilder(0, 1, 1).Build();
  }

  auto parts = Split(s, '-', 3);
  return runtime::Date::DateBuilder(std::stoi(parts[0]), std::stoi(parts[1]),
                                    std::stoi(parts[2]))
      .Build();
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

  std::string squote;
  if (s.size() >= 2 && s.front() == '\"' && s.back() == '\"') {
    squote = s;
  } else {
    squote = "\"" + s + "\"";
  }

  std::stringstream parser(squote);
  std::string output;
  parser >> std::quoted(output);
  return output;
}

std::mutex cout_mutex;
void Print(std::string_view x) {
  const std::lock_guard<std::mutex> lock(cout_mutex);
  std::cout << x << std::endl;
}

}  // namespace kush::util
