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

void aka_title() {
  /*
    CREATE TABLE aka_title (
        id INTEGER NOT NULL PRIMARY KEY,
        movie_id INTEGER NOT NULL,
        title TEXT NOT NULL,
        imdb_index TEXT,
        kind_id INTEGER NOT NULL,
        production_year INTEGER,
        phonetic_code TEXT,
        episode_of_id INTEGER,
        season_nr INTEGER,
        episode_nr INTEGER,
        note TEXT,
        md5sum TEXT
    );
  */

  DECLARE_NOT_NULL_COL(int32_t, id);
  DECLARE_NOT_NULL_COL(int32_t, movie_id);
  DECLARE_NOT_NULL_COL(std::string, title);
  DECLARE_NULL_COL(std::string, imdb_index);
  DECLARE_NOT_NULL_COL(int32_t, kind_id);
  DECLARE_NULL_COL(int32_t, production_year);
  DECLARE_NULL_COL(std::string, phonetic_code);
  DECLARE_NULL_COL(int32_t, episode_of_id);
  DECLARE_NULL_COL(int32_t, season_nr);
  DECLARE_NULL_COL(int32_t, episode_nr);
  DECLARE_NULL_COL(std::string, note);
  DECLARE_NULL_COL(std::string, md5sum);

  std::ifstream fin(raw + "aka_title.tbl");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|');

    APPEND_NOT_NULL(id, ParseInt32, data[0], tuple_idx);
    APPEND_NOT_NULL(movie_id, ParseInt32, data[1], tuple_idx);
    APPEND_NOT_NULL(title, ParseString, data[2], tuple_idx);
    APPEND_NULL(imdb_index, ParseString, data[3], tuple_idx);
    APPEND_NOT_NULL(kind_id, ParseInt32, data[4], tuple_idx);
    APPEND_NULL(production_year, ParseInt32, data[5], tuple_idx);
    APPEND_NULL(phonetic_code, ParseString, data[6], tuple_idx);
    APPEND_NULL(episode_of_id, ParseInt32, data[7], tuple_idx);
    APPEND_NULL(season_nr, ParseInt32, data[8], tuple_idx);
    APPEND_NULL(episode_nr, ParseInt32, data[9], tuple_idx);
    APPEND_NULL(note, ParseString, data[10], tuple_idx);
    APPEND_NULL(md5sum, ParseString, data[11], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL(int32_t, id, dest, "akat_id");
  SERIALIZE_NOT_NULL(int32_t, movie_id, dest, "akat_movie_id");
  SERIALIZE_NOT_NULL(std::string, title, dest, "akat_title");
  SERIALIZE_NULL(std::string, imdb_index, dest, "akat_imdb_index");
  SERIALIZE_NOT_NULL(int32_t, kind_id, dest, "akat_kind_id");
  SERIALIZE_NULL(int32_t, production_year, dest, "akat_production_year");
  SERIALIZE_NULL(std::string, phonetic_code, dest, "akat_phonetic_code");
  SERIALIZE_NULL(int32_t, episode_of_id, dest, "akat_episode_of_id");
  SERIALIZE_NULL(int32_t, season_nr, dest, "akat_season_nr");
  SERIALIZE_NULL(int32_t, episode_nr, dest, "akat_episode_nr");
  SERIALIZE_NULL(std::string, note, dest, "akat_note");
  SERIALIZE_NULL(std::string, md5sum, dest, "akat_md5sum");
  Print("aka_title complete");
}

void cast_info() {
  /*
    CREATE TABLE cast_info (
        id INTEGER NOT NULL PRIMARY KEY,
        person_id INTEGER NOT NULL,
        movie_id INTEGER NOT NULL,
        person_role_id INTEGER,
        note TEXT,
        nr_order INTEGER,
        role_id INTEGER NOT NULL
    );
  */

  DECLARE_NOT_NULL_COL(int32_t, id);
  DECLARE_NOT_NULL_COL(int32_t, person_id);
  DECLARE_NOT_NULL_COL(int32_t, movie_id);
  DECLARE_NULL_COL(int32_t, person_role_id);
  DECLARE_NULL_COL(std::string, note);
  DECLARE_NULL_COL(int32_t, nr_order);
  DECLARE_NOT_NULL_COL(int32_t, role_id);

  std::ifstream fin(raw + "cast_info.tbl");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|');

    APPEND_NOT_NULL(id, ParseInt32, data[0], tuple_idx);
    APPEND_NOT_NULL(person_id, ParseInt32, data[1], tuple_idx);
    APPEND_NOT_NULL(movie_id, ParseInt32, data[2], tuple_idx);
    APPEND_NULL(person_role_id, ParseInt32, data[3], tuple_idx);
    APPEND_NULL(note, ParseString, data[4], tuple_idx);
    APPEND_NULL(nr_order, ParseInt32, data[5], tuple_idx);
    APPEND_NOT_NULL(role_id, ParseInt32, data[6], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL(int32_t, id, dest, "ci_id");
  SERIALIZE_NOT_NULL(int32_t, person_id, dest, "ci_person_id");
  SERIALIZE_NOT_NULL(int32_t, movie_id, dest, "ci_movie_id");
  SERIALIZE_NULL(int32_t, person_role_id, dest, "ci_person_role_id");
  SERIALIZE_NULL(std::string, note, dest, "ci_note");
  SERIALIZE_NULL(int32_t, nr_order, dest, "ci_nr_order");
  SERIALIZE_NOT_NULL(int32_t, role_id, dest, "ci_role_id");
  Print("cast_info complete");
}

void char_name() {
  /*
    CREATE TABLE char_name (
        id INTEGER NOT NULL PRIMARY KEY,
        name TEXT NOT NULL,
        imdb_index TEXT,
        imdb_id INTEGER,
        name_pcode_nf TEXT,
        surname_pcode TEXT,
        md5sum TEXT
    );
  */

  DECLARE_NOT_NULL_COL(int32_t, id);
  DECLARE_NOT_NULL_COL(std::string, name);
  DECLARE_NULL_COL(std::string, imdb_index);
  DECLARE_NULL_COL(int32_t, imdb_id);
  DECLARE_NULL_COL(std::string, name_pcode_nf);
  DECLARE_NULL_COL(std::string, surname_pcode);
  DECLARE_NULL_COL(std::string, md5sum);

  std::ifstream fin(raw + "char_name.tbl");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, '|');

    APPEND_NOT_NULL(id, ParseInt32, data[0], tuple_idx);
    APPEND_NOT_NULL(name, ParseString, data[1], tuple_idx);
    APPEND_NULL(imdb_index, ParseString, data[2], tuple_idx);
    APPEND_NULL(imdb_id, ParseInt32, data[3], tuple_idx);
    APPEND_NULL(name_pcode_nf, ParseString, data[4], tuple_idx);
    APPEND_NULL(surname_pcode, ParseString, data[5], tuple_idx);
    APPEND_NULL(md5sum, ParseString, data[6], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL(int32_t, id, dest, "cn_id");
  SERIALIZE_NOT_NULL(std::string, name, dest, "cn_name");
  SERIALIZE_NULL(std::string, imdb_index, dest, "cn_imdb_index");
  SERIALIZE_NULL(int32_t, imdb_id, dest, "cn_imdb_id");
  SERIALIZE_NULL(std::string, name_pcode_nf, dest, "cn_name_pcode_nf");
  SERIALIZE_NULL(std::string, surname_pcode, dest, "cn_surname_pcode");
  SERIALIZE_NULL(std::string, md5sum, dest, "cn_md5sum");
  Print("char_name complete");
}

int main() { char_name(); }