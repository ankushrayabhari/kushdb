#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "absl/time/civil_time.h"
#include "absl/time/time.h"

#include "runtime/column_data.h"
#include "runtime/column_index.h"

#define DECLARE_COL_IDX(T, x)   \
  std::vector<T> x;             \
  std::vector<int8_t> x##_null; \
  std::unordered_map<T, std::vector<int32_t>> x##_index;

#define APPEND(x, parse, data, tuple_idx) \
  x.push_back(parse(data));               \
  x##_null.push_back(data.empty());       \
  if (!x##_null.back()) x##_index[x.back()].push_back(tuple_idx);

#define SERIALIZE(T, id, file)                                                 \
  kush::runtime::ColumnData::Serialize<T>("end_to_end_test/data/" file ".kdb", \
                                          id);                                 \
  kush::runtime::ColumnData::Serialize<int8_t>(                                \
      "end_to_end_test/data/" file "_null.kdb", id##_null);                    \
  kush::runtime::ColumnIndex::Serialize<T>(                                    \
      "end_to_end_test/data/" file ".kdbindex", id##_index);

std::vector<std::string> split(const std::string& s, char delim) {
  std::stringstream ss(s);
  std::string item;
  std::vector<std::string> elems;
  while (std::getline(ss, item, delim)) {
    elems.push_back(std::move(item));
  }
  return elems;
}

int64_t ParseDate(const std::string& s) {
  if (s.empty()) {
    return 0;
  }

  auto parts = split(s, '-');
  auto day = absl::CivilDay(std::stoi(parts[0]), std::stoi(parts[1]),
                            std::stoi(parts[2]));
  absl::TimeZone utc = absl::UTCTimeZone();
  absl::Time time = absl::FromCivil(day, utc);
  return absl::ToUnixMillis(time);
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

const std::string& ParseString(const std::string& s) { return s; }

void People() {
  /*
    CREATE TABLE people (
      id INTEGER,
      name TEXT,
    );
  */

  DECLARE_COL_IDX(int32_t, id);
  DECLARE_COL_IDX(std::string, name);

  std::ifstream fin("end_to_end_test/raw/people.tbl");
  int32_t tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = split(line, '|');

    APPEND(id, ParseInt32, data[0], tuple_idx);
    APPEND(name, ParseString, data[1], tuple_idx);
    tuple_idx++;
  }

  SERIALIZE(int32_t, id, "people_id");
  SERIALIZE(std::string, name, "people_name");
  std::cout << "people complete" << std::endl;
}

void Info() {
  /*
    CREATE TABLE info (
      id INTEGER,
      cheated BOOLEAN,
      date TIMESTAMP,
      zscore REAL,
      num1 SMALLINT,
      num2 BIGINT,
    );
  */

  DECLARE_COL_IDX(int32_t, id);
  DECLARE_COL_IDX(int8_t, cheated);
  DECLARE_COL_IDX(int64_t, date);
  DECLARE_COL_IDX(double, zscore);
  DECLARE_COL_IDX(int16_t, num1);
  DECLARE_COL_IDX(int64_t, num2);

  std::ifstream fin("end_to_end_test/raw/info.tbl");
  int32_t tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = split(line, '|');

    APPEND(id, ParseInt32, data[0], tuple_idx);
    APPEND(cheated, ParseBool, data[1], tuple_idx);
    APPEND(date, ParseDate, data[2], tuple_idx);
    APPEND(zscore, ParseDouble, data[3], tuple_idx);
    APPEND(num1, ParseInt16, data[4], tuple_idx);
    APPEND(num2, ParseInt64, data[5], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE(int32_t, id, "info_id");
  SERIALIZE(int8_t, cheated, "info_cheated");
  SERIALIZE(int64_t, date, "info_date");
  SERIALIZE(double, zscore, "info_zscore");
  SERIALIZE(int16_t, num1, "info_num1");
  SERIALIZE(int64_t, num2, "info_num2");
  std::cout << "info complete" << std::endl;
}

int main() {
  std::vector<std::function<void(void)>> loads{People, Info};
  for (auto& f : loads) {
    f();
  }
}
