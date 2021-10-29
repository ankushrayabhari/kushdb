#include "util/load.h"

#include <cstdint>
#include <execution>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>

using namespace kush::util;

std::string dest = "benchmark/job/data/";
std::string raw = "benchmark/job/raw/";

void aka_name() {
  /*
    CREATE TABLE aka_name (
        id INTEGER NOT NULL PRIMARY KEY,
        person_id INTEGER NOT NULL,
        name TEXT NOT NULL,
        imdb_index TEXT,
        name_pcode_cf TEXT,
        name_pcode_nf TEXT,
        surname_pcode TEXT,
        md5sum TEXT
    );
  */

  DECLARE_NOT_NULL_COL(int32_t, id);
  DECLARE_NOT_NULL_COL(int32_t, person_id);
  DECLARE_NOT_NULL_COL(std::string, name);
  DECLARE_NULL_COL(std::string, imdb_index);
  DECLARE_NULL_COL(std::string, name_pcode_cf);
  DECLARE_NULL_COL(std::string, name_pcode_nf);
  DECLARE_NULL_COL(std::string, surname_pcode);
  DECLARE_NULL_COL(std::string, md5sum);

  std::ifstream fin(raw + "aka_name.tbl");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|');

    APPEND_NOT_NULL(id, ParseInt32, data[0], tuple_idx);
    APPEND_NOT_NULL(person_id, ParseInt32, data[1], tuple_idx);
    APPEND_NOT_NULL(name, ParseString, data[2], tuple_idx);
    APPEND_NULL(imdb_index, ParseString, data[3], tuple_idx);
    APPEND_NULL(name_pcode_cf, ParseString, data[4], tuple_idx);
    APPEND_NULL(name_pcode_nf, ParseString, data[5], tuple_idx);
    APPEND_NULL(surname_pcode, ParseString, data[6], tuple_idx);
    APPEND_NULL(md5sum, ParseString, data[7], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL(int32_t, id, dest, "akan_id");
  SERIALIZE_NOT_NULL(int32_t, person_id, dest, "akan_person_id");
  SERIALIZE_NOT_NULL(std::string, name, dest, "akan_name");
  SERIALIZE_NULL(std::string, imdb_index, dest, "akan_imdb_index");
  SERIALIZE_NULL(std::string, name_pcode_cf, dest, "akan_name_pcode_cf");
  SERIALIZE_NULL(std::string, name_pcode_nf, dest, "akan_name_pcode_nf");
  SERIALIZE_NULL(std::string, surname_pcode, dest, "akan_surname_pcode");
  SERIALIZE_NULL(std::string, md5sum, dest, "akan_md5sum");
  Print("aka_name complete");
}

int main() { aka_name(); }