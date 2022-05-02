#include <cstdint>
#include <execution>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>

#include "util/load.h"

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

  DECLARE_NOT_NULL_ENUM_COL(name);
  DECLARE_NULL_ENUM_COL(imdb_index);
  DECLARE_NULL_ENUM_COL(name_pcode_cf);
  DECLARE_NULL_ENUM_COL(name_pcode_nf);
  DECLARE_NULL_ENUM_COL(surname_pcode);
  DECLARE_NULL_ENUM_COL(md5sum);

  std::ifstream fin(raw + "aka_name.csv");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, ',', 8);

    APPEND_NOT_NULL_ENUM(name, data[2], tuple_idx);
    APPEND_NULL_ENUM(imdb_index, data[3], tuple_idx);
    APPEND_NULL_ENUM(name_pcode_cf, data[4], tuple_idx);
    APPEND_NULL_ENUM(name_pcode_nf, data[5], tuple_idx);
    APPEND_NULL_ENUM(surname_pcode, data[6], tuple_idx);
    APPEND_NULL_ENUM(md5sum, data[7], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL_ENUM(name, dest, "akan_name_enum");
  SERIALIZE_NULL_ENUM(imdb_index, dest, "akan_imdb_index_enum");
  SERIALIZE_NULL_ENUM(name_pcode_cf, dest, "akan_name_pcode_cf_enum");
  SERIALIZE_NULL_ENUM(name_pcode_nf, dest, "akan_name_pcode_nf_enum");
  SERIALIZE_NULL_ENUM(surname_pcode, dest, "akan_surname_pcode_enum");
  SERIALIZE_NULL_ENUM(md5sum, dest, "akan_md5sum_enum");
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

  DECLARE_NOT_NULL_ENUM_COL(title);
  DECLARE_NULL_ENUM_COL(imdb_index);
  DECLARE_NULL_ENUM_COL(phonetic_code);
  DECLARE_NULL_ENUM_COL(note);
  DECLARE_NULL_ENUM_COL(md5sum);

  std::ifstream fin(raw + "aka_title.csv");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, ',', 12);

    APPEND_NOT_NULL_ENUM(title, data[2], tuple_idx);
    APPEND_NULL_ENUM(imdb_index, data[3], tuple_idx);
    APPEND_NULL_ENUM(phonetic_code, data[6], tuple_idx);
    APPEND_NULL_ENUM(note, data[10], tuple_idx);
    APPEND_NULL_ENUM(md5sum, data[11], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL_ENUM(title, dest, "akat_title_enum");
  SERIALIZE_NULL_ENUM(imdb_index, dest, "akat_imdb_index_enum");
  SERIALIZE_NULL_ENUM(phonetic_code, dest, "akat_phonetic_code_enum");
  SERIALIZE_NULL_ENUM(note, dest, "akat_note_enum");
  SERIALIZE_NULL_ENUM(md5sum, dest, "akat_md5sum_enum");
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

  DECLARE_NULL_ENUM_COL(note);

  std::ifstream fin(raw + "cast_info.csv");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, ',', 7);

    APPEND_NULL_ENUM(note, data[4], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NULL_ENUM(note, dest, "ci_note_enum");
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

  DECLARE_NOT_NULL_ENUM_COL(name);
  DECLARE_NULL_ENUM_COL(imdb_index);
  DECLARE_NULL_ENUM_COL(name_pcode_nf);
  DECLARE_NULL_ENUM_COL(surname_pcode);
  DECLARE_NULL_ENUM_COL(md5sum);

  std::ifstream fin(raw + "char_name.csv");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, ',', 7);

    APPEND_NOT_NULL_ENUM(name, data[1], tuple_idx);
    APPEND_NULL_ENUM(imdb_index, data[2], tuple_idx);
    APPEND_NULL_ENUM(name_pcode_nf, data[4], tuple_idx);
    APPEND_NULL_ENUM(surname_pcode, data[5], tuple_idx);
    APPEND_NULL_ENUM(md5sum, data[6], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL_ENUM(name, dest, "cn_name_enum");
  SERIALIZE_NULL_ENUM(imdb_index, dest, "cn_imdb_index_enum");
  SERIALIZE_NULL_ENUM(name_pcode_nf, dest, "cn_name_pcode_nf_enum");
  SERIALIZE_NULL_ENUM(surname_pcode, dest, "cn_surname_pcode_enum");
  SERIALIZE_NULL_ENUM(md5sum, dest, "cn_md5sum_enum");
  Print("char_name complete");
}

void comp_cast_type() {
  /*
    CREATE TABLE comp_cast_type (
        id INTEGER NOT NULL PRIMARY KEY,
        kind TEXT
    );
  */

  DECLARE_NULL_ENUM_COL(kind);

  std::ifstream fin(raw + "comp_cast_type.csv");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, ',', 2);

    APPEND_NULL_ENUM(kind, data[1], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NULL_ENUM(kind, dest, "cct_kind_enum");
  Print("comp_cast_type complete");
}

void company_name() {
  /*
    CREATE TABLE company_name (
        id INTEGER NOT NULL PRIMARY KEY,
        name TEXT NOT NULL,
        country_code TEXT,
        imdb_id INTEGER,
        name_pcode_nf TEXT,
        name_pcode_sf TEXT,
        md5sum TEXT
    );
  */

  DECLARE_NOT_NULL_ENUM_COL(name);
  DECLARE_NULL_ENUM_COL(country_code);
  DECLARE_NULL_ENUM_COL(name_pcode_nf);
  DECLARE_NULL_ENUM_COL(name_pcode_sf);
  DECLARE_NULL_ENUM_COL(md5sum);

  std::ifstream fin(raw + "company_name.csv");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, ',', 7);

    APPEND_NOT_NULL_ENUM(name, data[1], tuple_idx);
    APPEND_NULL_ENUM(country_code, data[2], tuple_idx);
    APPEND_NULL_ENUM(name_pcode_nf, data[4], tuple_idx);
    APPEND_NULL_ENUM(name_pcode_sf, data[5], tuple_idx);
    APPEND_NULL_ENUM(md5sum, data[6], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL_ENUM(name, dest, "cmpn_name_enum");
  SERIALIZE_NULL_ENUM(country_code, dest, "cmpn_country_code_enum");
  SERIALIZE_NULL_ENUM(name_pcode_nf, dest, "cmpn_name_pcode_nf_enum");
  SERIALIZE_NULL_ENUM(name_pcode_sf, dest, "cmpn_surname_pcode_enum");
  SERIALIZE_NULL_ENUM(md5sum, dest, "cmpn_md5sum_enum");
  Print("company_name complete");
}

void company_type() {
  /*
    CREATE TABLE company_type (
        id INTEGER NOT NULL PRIMARY KEY,
        kind TEXT
    );
  */

  DECLARE_NULL_ENUM_COL(kind);

  std::ifstream fin(raw + "company_type.csv");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, ',', 2);

    APPEND_NULL_ENUM(kind, data[1], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NULL_ENUM(kind, dest, "ct_kind_enum");
  Print("company_type complete");
}

void info_type() {
  /*
    CREATE TABLE info_type (
        id INTEGER NOT NULL PRIMARY KEY,
        info TEXT
    );
  */

  DECLARE_NULL_ENUM_COL(info);

  std::ifstream fin(raw + "info_type.csv");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, ',', 2);

    APPEND_NULL_ENUM(info, data[1], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NULL_ENUM(info, dest, "it_info_enum");
  Print("info_type complete");
}

void keyword() {
  /*
    CREATE TABLE keyword (
        id INTEGER NOT NULL PRIMARY KEY,
        keyword TEXT NOT NULL,
        phonetic_code TEXT
    );
  */

  DECLARE_NOT_NULL_ENUM_COL(keyword);
  DECLARE_NULL_ENUM_COL(phonetic_code);

  std::ifstream fin(raw + "keyword.csv");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, ',', 3);

    APPEND_NOT_NULL_ENUM(keyword, data[1], tuple_idx);
    APPEND_NULL_ENUM(phonetic_code, data[2], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL_ENUM(keyword, dest, "k_keyword_enum");
  SERIALIZE_NULL_ENUM(phonetic_code, dest, "k_phonetic_code_enum");
  Print("keyword complete");
}

void kind_type() {
  /*
    CREATE TABLE kind_type (
        id INTEGER NOT NULL PRIMARY KEY,
        kind TEXT
    );
  */

  DECLARE_NULL_ENUM_COL(kind);

  std::ifstream fin(raw + "kind_type.csv");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, ',', 2);

    APPEND_NULL_ENUM(kind, data[1], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NULL_ENUM(kind, dest, "kt_kind_enum");
  Print("kind_type complete");
}

void link_type() {
  /*
    CREATE TABLE link_type (
        id INTEGER NOT NULL PRIMARY KEY,
        link TEXT
    );
  */

  DECLARE_NULL_ENUM_COL(link);

  std::ifstream fin(raw + "link_type.csv");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, ',', 2);

    APPEND_NULL_ENUM(link, data[1], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NULL_ENUM(link, dest, "lt_link_enum");
  Print("link_type complete");
}

void movie_companies() {
  /*
    CREATE TABLE movie_companies (
        id INTEGER NOT NULL PRIMARY KEY,
        movie_id INTEGER NOT NULL,
        company_id INTEGER NOT NULL,
        company_type_id INTEGER NOT NULL,
        note TEXT
    );
  */

  DECLARE_NULL_ENUM_COL(note);

  std::ifstream fin(raw + "movie_companies.csv");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, ',', 5);

    APPEND_NULL_ENUM(note, data[4], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NULL_ENUM(note, dest, "mc_note_enum");
  Print("movie_companies complete");
}

void movie_info() {
  /*
    CREATE TABLE movie_info (
        id INTEGER NOT NULL PRIMARY KEY,
        movie_id INTEGER NOT NULL,
        info_type_id INTEGER NOT NULL,
        info TEXT NOT NULL,
        note TEXT
    );
  */

  DECLARE_NOT_NULL_ENUM_COL(info);
  DECLARE_NULL_ENUM_COL(note);

  std::ifstream fin(raw + "movie_info.csv");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, ',', 5);

    APPEND_NOT_NULL_ENUM(info, data[3], tuple_idx);
    APPEND_NULL_ENUM(note, data[4], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL_ENUM(info, dest, "mi_info_enum");
  SERIALIZE_NULL_ENUM(note, dest, "mi_note_enum");
  Print("movie_info complete");
}

void movie_info_idx() {
  /*
    CREATE TABLE movie_info_idx (
        id INTEGER NOT NULL PRIMARY KEY,
        movie_id INTEGER NOT NULL,
        info_type_id INTEGER NOT NULL,
        info TEXT NOT NULL,
        note TEXT
    );
  */

  DECLARE_NOT_NULL_ENUM_COL(info);
  DECLARE_NULL_ENUM_COL(note);

  std::ifstream fin(raw + "movie_info_idx.csv");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, ',', 5);

    APPEND_NOT_NULL_ENUM(info, data[3], tuple_idx);
    APPEND_NULL_ENUM(note, data[4], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL_ENUM(info, dest, "mii_info_enum");
  SERIALIZE_NULL_ENUM(note, dest, "mii_note_enum");
  Print("movie_info_idx complete");
}

void name() {
  /*
    CREATE TABLE name (
        id INTEGER NOT NULL PRIMARY KEY,
        name TEXT NOT NULL,
        imdb_index TEXT,
        imdb_id INTEGER,
        gender TEXT,
        name_pcode_cf TEXT,
        name_pcode_nf TEXT,
        surname_pcode TEXT,
        md5sum TEXT
    );
  */

  DECLARE_NOT_NULL_ENUM_COL(name);
  DECLARE_NULL_ENUM_COL(imdb_index);
  DECLARE_NULL_ENUM_COL(gender);
  DECLARE_NULL_ENUM_COL(name_pcode_cf);
  DECLARE_NULL_ENUM_COL(name_pcode_nf);
  DECLARE_NULL_ENUM_COL(surname_pcode);
  DECLARE_NULL_ENUM_COL(md5sum);

  std::ifstream fin(raw + "name.csv");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, ',', 9);

    APPEND_NOT_NULL_ENUM(name, data[1], tuple_idx);
    APPEND_NULL_ENUM(imdb_index, data[2], tuple_idx);
    APPEND_NULL_ENUM(gender, data[4], tuple_idx);
    APPEND_NULL_ENUM(name_pcode_cf, data[5], tuple_idx);
    APPEND_NULL_ENUM(name_pcode_nf, data[6], tuple_idx);
    APPEND_NULL_ENUM(surname_pcode, data[7], tuple_idx);
    APPEND_NULL_ENUM(md5sum, data[8], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL_ENUM(name, dest, "n_name_enum");
  SERIALIZE_NULL_ENUM(imdb_index, dest, "n_imdb_index_enum");
  SERIALIZE_NULL_ENUM(gender, dest, "n_gender_enum");
  SERIALIZE_NULL_ENUM(name_pcode_cf, dest, "n_name_pcode_cf_enum");
  SERIALIZE_NULL_ENUM(name_pcode_nf, dest, "n_name_pcode_nf_enum");
  SERIALIZE_NULL_ENUM(surname_pcode, dest, "n_surname_pcode_enum");
  SERIALIZE_NULL_ENUM(md5sum, dest, "n_md5sum_enum");
  Print("name complete");
}

void person_info() {
  /*
    CREATE TABLE person_info (
        id INTEGER NOT NULL PRIMARY KEY,
        person_id INTEGER NOT NULL,
        info_type_id INTEGER NOT NULL,
        info TEXT NOT NULL,
        note TEXT
    );
  */

  DECLARE_NOT_NULL_ENUM_COL(info);
  DECLARE_NULL_ENUM_COL(note);

  std::ifstream fin(raw + "person_info.csv");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, ',', 5);

    APPEND_NOT_NULL_ENUM(info, data[3], tuple_idx);
    APPEND_NULL_ENUM(note, data[4], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL_ENUM(info, dest, "pi_info_enum");
  SERIALIZE_NULL_ENUM(note, dest, "pi_note_enum");
  Print("person_info complete");
}

void role_type() {
  /*
    CREATE TABLE role_type (
        id INTEGER NOT NULL PRIMARY KEY,
        role TEXT
    );
  */

  DECLARE_NULL_ENUM_COL(role);

  std::ifstream fin(raw + "role_type.csv");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, ',', 2);

    APPEND_NULL_ENUM(role, data[1], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NULL_ENUM(role, dest, "r_role_enum");
  Print("role_type complete");
}

void title() {
  /*
    CREATE TABLE title (
        id INTEGER NOT NULL PRIMARY KEY,
        title TEXT NOT NULL,
        imdb_index TEXT,
        kind_id INTEGER NOT NULL,
        production_year INTEGER,
        imdb_id INTEGER,
        phonetic_code TEXT,
        episode_of_id INTEGER,
        season_nr INTEGER,
        episode_nr INTEGER,
        series_years TEXT,
        md5sum TEXT
    );
  */

  DECLARE_NOT_NULL_ENUM_COL(title);
  DECLARE_NULL_ENUM_COL(imdb_index);
  DECLARE_NULL_ENUM_COL(phonetic_code);
  DECLARE_NULL_ENUM_COL(series_years);
  DECLARE_NULL_ENUM_COL(md5sum);

  std::ifstream fin(raw + "title.csv");
  int tuple_idx = 0;
  for (std::string line; std::getline(fin, line);) {
    auto data = Split(line, ',', 12);

    APPEND_NOT_NULL_ENUM(title, data[1], tuple_idx);
    APPEND_NULL_ENUM(imdb_index, data[2], tuple_idx);
    APPEND_NULL_ENUM(phonetic_code, data[6], tuple_idx);
    APPEND_NULL_ENUM(series_years, data[10], tuple_idx);
    APPEND_NULL_ENUM(md5sum, data[11], tuple_idx);

    tuple_idx++;
  }

  SERIALIZE_NOT_NULL_ENUM(title, dest, "t_title_enum");
  SERIALIZE_NULL_ENUM(imdb_index, dest, "t_imdb_index_enum");
  SERIALIZE_NULL_ENUM(phonetic_code, dest, "t_phonetic_code_enum");
  SERIALIZE_NULL_ENUM(series_years, dest, "t_series_years_enum");
  SERIALIZE_NULL_ENUM(md5sum, dest, "t_md5sum_enum");
  Print("title complete");
}

int main() {
  aka_name();
  aka_title();
  cast_info();
  char_name();
  comp_cast_type();
  company_name();
  company_type();
  info_type();
  keyword();
  kind_type();
  link_type();
  movie_companies();
  movie_info();
  movie_info_idx();
  name();
  person_info();
  role_type();
  title();
}