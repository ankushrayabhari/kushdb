#include <algorithm>
#include <fstream>
#include <iostream>
#include <limits>
#include <memory>
#include <random>
#include <sstream>
#include <string>

#include "absl/time/civil_time.h"
#include "absl/time/time.h"

#include "runtime/column_data.h"

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

void People() {
  /*
    CREATE TABLE people (
      id INTEGER,
      name TEXT,
    );
  */

  std::vector<int32_t> id;
  std::vector<int8_t> id_null;
  std::vector<std::string> name;
  std::vector<int8_t> name_null;

  std::ifstream fin("end_to_end_test/raw/people.tbl");
  for (std::string line; std::getline(fin, line);) {
    auto data = split(line, '|');

    id.push_back(ParseInt32(data[0]));
    id_null.push_back(data[0].empty());
    name.push_back(data[1]);
    name_null.push_back(data[1].empty());
  }

  kush::runtime::ColumnData::Serialize<int32_t>(
      "end_to_end_test/data/people_id.kdb", id);
  kush::runtime::ColumnData::Serialize<std::string>(
      "end_to_end_test/data/people_name.kdb", name);

  kush::runtime::ColumnData::Serialize<int8_t>(
      "end_to_end_test/data/people_id_null.kdb", id_null);
  kush::runtime::ColumnData::Serialize<int8_t>(
      "end_to_end_test/data/people_name_null.kdb", name_null);
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

  std::vector<int32_t> id;
  std::vector<int8_t> id_null;
  std::vector<int8_t> cheated;
  std::vector<int8_t> cheated_null;
  std::vector<int64_t> date;
  std::vector<int8_t> date_null;
  std::vector<double> zscore;
  std::vector<int8_t> zscore_null;
  std::vector<int16_t> num1;
  std::vector<int8_t> num1_null;
  std::vector<int64_t> num2;
  std::vector<int8_t> num2_null;

  std::ifstream fin("end_to_end_test/raw/info.tbl");
  for (std::string line; std::getline(fin, line);) {
    auto data = split(line, '|');

    id.push_back(ParseInt32(data[0]));
    cheated.push_back(ParseBool(data[1]));
    date.push_back(ParseDate(data[2]));
    zscore.push_back(ParseDouble(data[3]));
    num1.push_back(ParseInt16(data[4]));
    num2.push_back(ParseInt64(data[5]));

    id_null.push_back(data[0].empty());
    cheated_null.push_back(data[1].empty());
    date_null.push_back(data[2].empty());
    zscore_null.push_back(data[3].empty());
    num1_null.push_back(data[4].empty());
    num2_null.push_back(data[5].empty());
  }

  kush::runtime::ColumnData::Serialize<int32_t>(
      "end_to_end_test/data/info_id.kdb", id);
  kush::runtime::ColumnData::Serialize<int8_t>(
      "end_to_end_test/data/info_cheated.kdb", cheated);
  kush::runtime::ColumnData::Serialize<int64_t>(
      "end_to_end_test/data/info_date.kdb", date);
  kush::runtime::ColumnData::Serialize<double>(
      "end_to_end_test/data/info_zscore.kdb", zscore);
  kush::runtime::ColumnData::Serialize<int16_t>(
      "end_to_end_test/data/info_num1.kdb", num1);
  kush::runtime::ColumnData::Serialize<int64_t>(
      "end_to_end_test/data/info_num2.kdb", num2);

  kush::runtime::ColumnData::Serialize<int8_t>(
      "end_to_end_test/data/info_id_null.kdb", id_null);
  kush::runtime::ColumnData::Serialize<int8_t>(
      "end_to_end_test/data/info_cheated_null.kdb", cheated_null);
  kush::runtime::ColumnData::Serialize<int8_t>(
      "end_to_end_test/data/info_date_null.kdb", date_null);
  kush::runtime::ColumnData::Serialize<int8_t>(
      "end_to_end_test/data/info_zscore_null.kdb", zscore_null);
  kush::runtime::ColumnData::Serialize<int8_t>(
      "end_to_end_test/data/info_num1_null.kdb", num1_null);
  kush::runtime::ColumnData::Serialize<int8_t>(
      "end_to_end_test/data/info_num2_null.kdb", num2_null);
  std::cout << "info complete" << std::endl;
}

int main() {
  std::vector<std::function<void(void)>> loads{People, Info};
  for (auto& f : loads) {
    f();
  }
}
