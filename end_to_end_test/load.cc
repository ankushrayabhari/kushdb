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

#include "runtime/column_data_builder.h"

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
  auto parts = split(s, '-');
  auto day = absl::CivilDay(std::stoi(parts[0]), std::stoi(parts[1]),
                            std::stoi(parts[2]));
  absl::TimeZone utc = absl::UTCTimeZone();
  absl::Time time = absl::FromCivil(day, utc);
  return absl::ToUnixMillis(time);
}

void People() {
  /*
    CREATE TABLE people (
      id INTEGER NOT NULL,
      name TEXT NOT NULL,
    );
  */

  std::vector<int32_t> id;
  std::vector<std::string> name;

  std::ifstream fin("end_to_end_test/raw/people.tbl");
  for (std::string line; std::getline(fin, line);) {
    auto data = split(line, '|');

    id.push_back(std::stoi(data[0]));
    name.push_back(data[1]);
  }

  kush::runtime::ColumnData::Serialize<int32_t>(
      "end_to_end_test/data/people_id.kdb", id);
  kush::runtime::ColumnData::Serialize<std::string>(
      "end_to_end_test/data/people_name.kdb", name);
  std::cout << "people complete" << std::endl;
}

void Info() {
  /*
    CREATE TABLE info (
      id INTEGER NOT NULL,
      cheated BOOLEAN NOT NULL,
      date TIMESTAMP NOT NULL,
      zscore REAL NOT NULL,
      num1 SMALLINT NOT NULL,
      num2 BIGINT NOT NULL,
    );
  */

  std::vector<int32_t> id;
  std::vector<int8_t> cheated;
  std::vector<int64_t> date;
  std::vector<double> zscore;
  std::vector<int16_t> num1;
  std::vector<int64_t> num2;

  std::ifstream fin("end_to_end_test/raw/info.tbl");
  for (std::string line; std::getline(fin, line);) {
    auto data = split(line, '|');

    id.push_back(std::stoi(data[0]));
    cheated.push_back(data[1] == "true" ? 1 : 0);
    date.push_back(ParseDate(data[2]));
    zscore.push_back(std::stod(data[3]));
    num1.push_back(std::stoi(data[4]));
    num2.push_back(std::stol(data[5]));
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
  std::cout << "info complete" << std::endl;
}

int main() {
  std::vector<std::function<void(void)>> loads{People, Info};
  for (auto& f : loads) {
    f();
  }
}
