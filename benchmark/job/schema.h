#pragma once

#include "catalog/catalog.h"
#include "catalog/sql_type.h"

kush::catalog::Database Schema() {
  using namespace kush::catalog;
  Database db;

  {
    auto& table = db.Insert("aka_name");
    table.Insert("id", SqlType::INT, "benchmark/job/data/akan_id.kdb", "",
                 "benchmark/job/data/akan_id.kdbindex");
    table.Insert("person_id", SqlType::INT,
                 "benchmark/job/data/akan_person_id.kdb", "",
                 "benchmark/job/data/akan_person_id.kdbindex");
    table.Insert("name", SqlType::TEXT, "benchmark/job/data/akan_name.kdb", "",
                 "benchmark/job/data/akan_name.kdbindex");
    table.Insert("imdb_index", SqlType::TEXT,
                 "benchmark/job/data/akan_imdb_index.kdb",
                 "benchmark/job/data/akan_imdb_index_null.kdb",
                 "benchmark/job/data/akan_imdb_index.kdbindex");
    table.Insert("name_pcode_cf", SqlType::TEXT,
                 "benchmark/job/data/akan_name_pcode_cf.kdb",
                 "benchmark/job/data/akan_name_pcode_cf_null.kdb",
                 "benchmark/job/data/akan_name_pcode_cf.kdbindex");
    table.Insert("name_pcode_nf", SqlType::TEXT,
                 "benchmark/job/data/akan_name_pcode_nf.kdb",
                 "benchmark/job/data/akan_name_pcode_nf_null.kdb",
                 "benchmark/job/data/akan_name_pcode_nf.kdbindex");
    table.Insert("surname_pcode", SqlType::TEXT,
                 "benchmark/job/data/akan_surname_pcode.kdb",
                 "benchmark/job/data/akan_surname_pcode_null.kdb",
                 "benchmark/job/data/akan_surname_pcode.kdbindex");
    table.Insert("md5sum", SqlType::TEXT, "benchmark/job/data/akan_md5sum.kdb",
                 "benchmark/job/data/akan_md5sum_null.kdb",
                 "benchmark/job/data/akan_md5sum.kdbindex");
  }

  {
    auto& table = db.Insert("aka_title");
    table.Insert("id", SqlType::INT, "benchmark/job/data/akat_id.kdb", "",
                 "benchmark/job/data/akat_id.kdbindex");
    table.Insert("movie_id", SqlType::INT,
                 "benchmark/job/data/akat_movie_id.kdb", "",
                 "benchmark/job/data/akat_movie_id.kdbindex");
    table.Insert("title", SqlType::TEXT, "benchmark/job/data/akat_title.kdb",
                 "", "benchmark/job/data/akat_title.kdbindex");
    table.Insert("imdb_index", SqlType::TEXT,
                 "benchmark/job/data/akat_imdb_index.kdb",
                 "benchmark/job/data/akat_imdb_index_null.kdb",
                 "benchmark/job/data/akat_imdb_index.kdbindex");
    table.Insert("kind_id", SqlType::INT, "benchmark/job/data/akat_kind_id.kdb",
                 "", "benchmark/job/data/akat_kind_id.kdbindex");
    table.Insert("production_year", SqlType::INT,
                 "benchmark/job/data/akat_production_year.kdb",
                 "benchmark/job/data/akat_production_year_null.kdb",
                 "benchmark/job/data/akat_production_year.kdbindex");
    table.Insert("phonetic_code", SqlType::TEXT,
                 "benchmark/job/data/akat_phonetic_code.kdb",
                 "benchmark/job/data/akat_phonetic_code_null.kdb",
                 "benchmark/job/data/akat_phonetic_code.kdbindex");
    table.Insert("episode_of_id", SqlType::INT,
                 "benchmark/job/data/akat_episode_of_id.kdb",
                 "benchmark/job/data/akat_episode_of_id_null.kdb",
                 "benchmark/job/data/akat_episode_of_id.kdbindex");
    table.Insert("season_nr", SqlType::INT,
                 "benchmark/job/data/akat_season_nr.kdb",
                 "benchmark/job/data/akat_season_nr_null.kdb",
                 "benchmark/job/data/akat_season_nr.kdbindex");
    table.Insert("episode_nr", SqlType::INT,
                 "benchmark/job/data/akat_episode_nr.kdb",
                 "benchmark/job/data/akat_episode_nr_null.kdb",
                 "benchmark/job/data/akat_episode_nr.kdbindex");
    table.Insert("note", SqlType::TEXT, "benchmark/job/data/akat_note.kdb",
                 "benchmark/job/data/akat_note_null.kdb",
                 "benchmark/job/data/akat_note.kdbindex");
    table.Insert("md5sum", SqlType::TEXT, "benchmark/job/data/akat_md5sum.kdb",
                 "benchmark/job/data/akat_md5sum_null.kdb",
                 "benchmark/job/data/akat_md5sum.kdbindex");
  }

  {
    auto& table = db.Insert("cast_info");
    table.Insert("id", SqlType::INT, "benchmark/job/data/ci_id.kdb", "",
                 "benchmark/job/data/ci_id.kdbindex");
    table.Insert("person_id", SqlType::INT,
                 "benchmark/job/data/ci_person_id.kdb", "",
                 "benchmark/job/data/ci_person_id.kdbindex");
    table.Insert("movie_id", SqlType::INT, "benchmark/job/data/ci_movie_id.kdb",
                 "", "benchmark/job/data/ci_movie_id.kdbindex");
    table.Insert("person_role_id", SqlType::INT,
                 "benchmark/job/data/ci_person_role_id.kdb",
                 "benchmark/job/data/ci_person_role_id_null.kdb",
                 "benchmark/job/data/ci_person_role_id.kdbindex");
    table.Insert("note", SqlType::TEXT, "benchmark/job/data/ci_note.kdb",
                 "benchmark/job/data/ci_note_null.kdb",
                 "benchmark/job/data/ci_note.kdbindex");
    table.Insert("nr_order", SqlType::INT, "benchmark/job/data/ci_nr_order.kdb",
                 "benchmark/job/data/ci_nr_order_null.kdb",
                 "benchmark/job/data/ci_nr_order.kdbindex");
    table.Insert("role_id", SqlType::TEXT, "benchmark/job/data/ci_role_id.kdb",
                 "", "benchmark/job/data/ci_role_id.kdbindex");
  }

  {
    auto& table = db.Insert("char_name");
    table.Insert("id", SqlType::INT, "benchmark/job/data/cn_id.kdb", "",
                 "benchmark/job/data/cn_id.kdbindex");
    table.Insert("name", SqlType::TEXT, "benchmark/job/data/cn_name.kdb", "",
                 "benchmark/job/data/cn_name.kdbindex");
    table.Insert("imdb_index", SqlType::TEXT,
                 "benchmark/job/data/cn_imdb_index.kdb",
                 "benchmark/job/data/cn_imdb_index_null.kdb",
                 "benchmark/job/data/cn_imdb_index.kdbindex");
    table.Insert("imdb_id", SqlType::INT, "benchmark/job/data/cn_imdb_id.kdb",
                 "benchmark/job/data/cn_imdb_id_null.kdb",
                 "benchmark/job/data/cn_imdb_id.kdbindex");
    table.Insert("name_pcode_nf", SqlType::TEXT,
                 "benchmark/job/data/cn_name_pcode_nf.kdb",
                 "benchmark/job/data/cn_name_pcode_nf_null.kdb",
                 "benchmark/job/data/cn_name_pcode_nf.kdbindex");
    table.Insert("surname_pcode", SqlType::TEXT,
                 "benchmark/job/data/cn_surname_pcode.kdb",
                 "benchmark/job/data/cn_surname_pcode_null.kdb",
                 "benchmark/job/data/cn_surname_pcode.kdbindex");
    table.Insert("md5sum", SqlType::TEXT, "benchmark/job/data/cn_md5sum.kdb",
                 "benchmark/job/data/cn_md5sum_null.kdb",
                 "benchmark/job/data/cn_md5sum.kdbindex");
  }

  {
    auto& table = db.Insert("comp_cast_type");
    table.Insert("id", SqlType::INT, "benchmark/job/data/cct_id.kdb", "",
                 "benchmark/job/data/cct_id.kdbindex");
    table.Insert("imdb_index", SqlType::TEXT,
                 "benchmark/job/data/cct_imdb_index.kdb",
                 "benchmark/job/data/cct_imdb_index_null.kdb",
                 "benchmark/job/data/cct_imdb_index.kdbindex");
  }

  {
    auto& table = db.Insert("company_name");
    table.Insert("id", SqlType::INT, "benchmark/job/data/cmpn_id.kdb", "",
                 "benchmark/job/data/cmpn_id.kdbindex");
    table.Insert("name", SqlType::TEXT, "benchmark/job/data/cmpn_name.kdb", "",
                 "benchmark/job/data/cmpn_name.kdbindex");
    table.Insert("country_code", SqlType::TEXT,
                 "benchmark/job/data/cmpn_country_code.kdb",
                 "benchmark/job/data/cmpn_country_code_null.kdb",
                 "benchmark/job/data/cmpn_country_code.kdbindex");
    table.Insert("imdb_id", SqlType::INT, "benchmark/job/data/cmpn_imdb_id.kdb",
                 "benchmark/job/data/cmpn_imdb_id_null.kdb",
                 "benchmark/job/data/cmpn_imdb_id.kdbindex");
    table.Insert("name_pcode_nf", SqlType::TEXT,
                 "benchmark/job/data/cmpn_name_pcode_nf.kdb",
                 "benchmark/job/data/cmpn_name_pcode_nf_null.kdb",
                 "benchmark/job/data/cmpn_name_pcode_nf.kdbindex");
    table.Insert("surname_pcode", SqlType::TEXT,
                 "benchmark/job/data/cmpn_surname_pcode.kdb",
                 "benchmark/job/data/cmpn_surname_pcode_null.kdb",
                 "benchmark/job/data/cmpn_surname_pcode.kdbindex");
    table.Insert("md5sum", SqlType::TEXT, "benchmark/job/data/cmpn_md5sum.kdb",
                 "benchmark/job/data/cmpn_md5sum_null.kdb",
                 "benchmark/job/data/cmpn_md5sum.kdbindex");
  }

  {
    auto& table = db.Insert("company_type");
    table.Insert("id", SqlType::INT, "benchmark/job/data/ct_id.kdb", "",
                 "benchmark/job/data/ct_id.kdbindex");
    table.Insert("kind", SqlType::TEXT, "benchmark/job/data/ct_kind.kdb",
                 "benchmark/job/data/ct_kind_null.kdb",
                 "benchmark/job/data/ct_kind.kdbindex");
  }

  {
    auto& table = db.Insert("complete_cast");
    table.Insert("id", SqlType::INT, "benchmark/job/data/cc_id.kdb", "",
                 "benchmark/job/data/cc_id.kdbindex");
    table.Insert("movie_id", SqlType::INT, "benchmark/job/data/cc_movie_id.kdb",
                 "benchmark/job/data/cc_movie_id_null.kdb",
                 "benchmark/job/data/cc_movie_id.kdbindex");
    table.Insert("subject_id", SqlType::INT,
                 "benchmark/job/data/cc_subject_id.kdb", "",
                 "benchmark/job/data/cc_subject_id.kdbindex");
    table.Insert("status_id", SqlType::INT,
                 "benchmark/job/data/cc_status_id.kdb", "",
                 "benchmark/job/data/cc_status_id.kdbindex");
  }

  {
    auto& table = db.Insert("info_type");
    table.Insert("id", SqlType::INT, "benchmark/job/data/it_id.kdb", "",
                 "benchmark/job/data/it_id.kdbindex");
    table.Insert("info", SqlType::TEXT, "benchmark/job/data/it_info.kdb",
                 "benchmark/job/data/it_info_null.kdb",
                 "benchmark/job/data/it_info.kdbindex");
  }

  {
    auto& table = db.Insert("keyword");
    table.Insert("id", SqlType::INT, "benchmark/job/data/k_id.kdb", "",
                 "benchmark/job/data/k_id.kdbindex");
    table.Insert("keyword", SqlType::TEXT, "benchmark/job/data/k_keyword.kdb",
                 "", "benchmark/job/data/k_keyword.kdbindex");
    table.Insert("phonetic_code", SqlType::TEXT,
                 "benchmark/job/data/k_phonetic_code.kdb",
                 "benchmark/job/data/k_phonetic_code_null.kdb",
                 "benchmark/job/data/k_phonetic_code.kdbindex");
  }

  {
    auto& table = db.Insert("kind_type");
    table.Insert("id", SqlType::INT, "benchmark/job/data/kt_id.kdb", "",
                 "benchmark/job/data/kt_id.kdbindex");
    table.Insert("kind", SqlType::TEXT, "benchmark/job/data/kt_kind.kdb",
                 "benchmark/job/data/kt_kind_null.kdb",
                 "benchmark/job/data/kt_kind.kdbindex");
  }

  {
    auto& table = db.Insert("link_type");
    table.Insert("id", SqlType::INT, "benchmark/job/data/lt_id.kdb", "",
                 "benchmark/job/data/lt_id.kdbindex");
    table.Insert("link", SqlType::TEXT, "benchmark/job/data/lt_link.kdb",
                 "benchmark/job/data/lt_link_null.kdb",
                 "benchmark/job/data/lt_link.kdbindex");
  }

  {
    auto& table = db.Insert("movie_companies");
    table.Insert("id", SqlType::INT, "benchmark/job/data/mc_id.kdb", "",
                 "benchmark/job/data/mc_id.kdbindex");
    table.Insert("movie_id", SqlType::INT, "benchmark/job/data/mc_movie_id.kdb",
                 "", "benchmark/job/data/mc_movie_id.kdbindex");
    table.Insert("company_id", SqlType::INT,
                 "benchmark/job/data/mc_company_id.kdb", "",
                 "benchmark/job/data/mc_company_id.kdbindex");
    table.Insert("company_type_id", SqlType::INT,
                 "benchmark/job/data/mc_company_type_id.kdb", "",
                 "benchmark/job/data/mc_company_type_id.kdbindex");
    table.Insert("note", SqlType::TEXT, "benchmark/job/data/mc_note.kdb",
                 "benchmark/job/data/mc_note_null.kdb",
                 "benchmark/job/data/mc_note.kdbindex");
  }

  {
    auto& table = db.Insert("movie_info");
    table.Insert("id", SqlType::INT, "benchmark/job/data/mi_id.kdb", "",
                 "benchmark/job/data/mi_id.kdbindex");
    table.Insert("movie_id", SqlType::INT, "benchmark/job/data/mi_movie_id.kdb",
                 "", "benchmark/job/data/mi_movie_id.kdbindex");
    table.Insert("info_type_id", SqlType::INT,
                 "benchmark/job/data/mi_info_type_id.kdb", "",
                 "benchmark/job/data/mi_info_type_id.kdbindex");
    table.Insert("info", SqlType::TEXT, "benchmark/job/data/mi_info.kdb", "",
                 "benchmark/job/data/mi_info.kdbindex");
    table.Insert("note", SqlType::TEXT, "benchmark/job/data/mi_note.kdb",
                 "benchmark/job/data/mi_note_null.kdb",
                 "benchmark/job/data/mi_note.kdbindex");
  }

  {
    auto& table = db.Insert("movie_info_idx");
    table.Insert("id", SqlType::INT, "benchmark/job/data/mii_id.kdb", "",
                 "benchmark/job/data/mii_id.kdbindex");
    table.Insert("movie_id", SqlType::INT,
                 "benchmark/job/data/mii_movie_id.kdb", "",
                 "benchmark/job/data/mii_movie_id.kdbindex");
    table.Insert("info_type_id", SqlType::INT,
                 "benchmark/job/data/mii_info_type_id.kdb", "",
                 "benchmark/job/data/mii_info_type_id.kdbindex");
    table.Insert("info", SqlType::TEXT, "benchmark/job/data/mii_info.kdb", "",
                 "benchmark/job/data/mii_info.kdbindex");
    table.Insert("note", SqlType::TEXT, "benchmark/job/data/mii_note.kdb",
                 "benchmark/job/data/mii_note_null.kdb",
                 "benchmark/job/data/mii_note.kdbindex");
  }

  {
    auto& table = db.Insert("movie_keyword");
    table.Insert("id", SqlType::INT, "benchmark/job/data/mk_id.kdb", "",
                 "benchmark/job/data/mk_id.kdbindex");
    table.Insert("movie_id", SqlType::INT, "benchmark/job/data/mk_movie_id.kdb",
                 "", "benchmark/job/data/mk_movie_id.kdbindex");
    table.Insert("keyword_id", SqlType::INT,
                 "benchmark/job/data/mk_keyword_id.kdb", "",
                 "benchmark/job/data/mk_keyword_id.kdbindex");
  }

  return db;
}