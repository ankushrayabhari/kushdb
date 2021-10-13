
#include "util/load.h"

#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>

using namespace kush::util;

constexpr std::string_view dest("end_to_end_test/data/");

void People() {
  /*
    CREATE TABLE people (
      id INTEGER,
      name TEXT,
    );
  */

  DECLARE_NULL_COL(int32_t, id);
  DECLARE_NULL_COL(std::string, name);

  std::ifstream fin("end_to_end_test/raw/people.tbl");
  int32_t tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|');

    APPEND_NULL(id, ParseInt32, data[0], tuple_idx);
    APPEND_NULL(name, ParseString, data[1], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NULL(int32_t, id, dest, "people_id");
  SERIALIZE_NULL(std::string, name, dest, "people_name");
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

  DECLARE_NULL_COL(int32_t, id);
  DECLARE_NULL_COL(int8_t, cheated);
  DECLARE_NULL_COL(int64_t, date);
  DECLARE_NULL_COL(double, zscore);
  DECLARE_NULL_COL(int16_t, num1);
  DECLARE_NULL_COL(int64_t, num2);

  std::ifstream fin("end_to_end_test/raw/info.tbl");
  int32_t tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|');

    APPEND_NULL(id, ParseInt32, data[0], tuple_idx);
    APPEND_NULL(cheated, ParseBool, data[1], tuple_idx);
    APPEND_NULL(date, ParseDate, data[2], tuple_idx);
    APPEND_NULL(zscore, ParseDouble, data[3], tuple_idx);
    APPEND_NULL(num1, ParseInt16, data[4], tuple_idx);
    APPEND_NULL(num2, ParseInt64, data[5], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NULL(int32_t, id, dest, "info_id");
  SERIALIZE_NULL(int8_t, cheated, dest, "info_cheated");
  SERIALIZE_NULL(int64_t, date, dest, "info_date");
  SERIALIZE_NULL(double, zscore, dest, "info_zscore");
  SERIALIZE_NULL(int16_t, num1, dest, "info_num1");
  SERIALIZE_NULL(int64_t, num2, dest, "info_num2");
  std::cout << "info complete" << std::endl;
}

int main() {
  std::vector<std::function<void(void)>> loads{People, Info};
  for (auto& f : loads) {
    f();
  }
}
