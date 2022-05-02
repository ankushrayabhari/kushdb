#pragma once

#include "absl/flags/flag.h"

#include "catalog/catalog.h"
#include "catalog/sql_type.h"
#include "runtime/enum.h"

ABSL_FLAG(bool, use_dictionary, false, "Use dictionary encoding.");

kush::catalog::Database RegularSchema() {
  using namespace kush::catalog;
  Database db;

  {
    auto& table = db.Insert("aka_name");
    table.Insert("id", Type::Int(), "benchmark/job/data/akan_id.kdb", "",
                 "benchmark/job/data/akan_id.kdbindex");
    table.Insert("person_id", Type::Int(),
                 "benchmark/job/data/akan_person_id.kdb", "",
                 "benchmark/job/data/akan_person_id.kdbindex");
    table.Insert("name", Type::Text(), "benchmark/job/data/akan_name.kdb", "",
                 "benchmark/job/data/akan_name.kdbindex");
    table.Insert("imdb_index", Type::Text(),
                 "benchmark/job/data/akan_imdb_index.kdb",
                 "benchmark/job/data/akan_imdb_index_null.kdb",
                 "benchmark/job/data/akan_imdb_index.kdbindex");
    table.Insert("name_pcode_cf", Type::Text(),
                 "benchmark/job/data/akan_name_pcode_cf.kdb",
                 "benchmark/job/data/akan_name_pcode_cf_null.kdb",
                 "benchmark/job/data/akan_name_pcode_cf.kdbindex");
    table.Insert("name_pcode_nf", Type::Text(),
                 "benchmark/job/data/akan_name_pcode_nf.kdb",
                 "benchmark/job/data/akan_name_pcode_nf_null.kdb",
                 "benchmark/job/data/akan_name_pcode_nf.kdbindex");
    table.Insert("surname_pcode", Type::Text(),
                 "benchmark/job/data/akan_surname_pcode.kdb",
                 "benchmark/job/data/akan_surname_pcode_null.kdb",
                 "benchmark/job/data/akan_surname_pcode.kdbindex");
    table.Insert("md5sum", Type::Text(), "benchmark/job/data/akan_md5sum.kdb",
                 "benchmark/job/data/akan_md5sum_null.kdb",
                 "benchmark/job/data/akan_md5sum.kdbindex");
  }

  {
    auto& table = db.Insert("aka_title");
    table.Insert("id", Type::Int(), "benchmark/job/data/akat_id.kdb", "",
                 "benchmark/job/data/akat_id.kdbindex");
    table.Insert("movie_id", Type::Int(),
                 "benchmark/job/data/akat_movie_id.kdb", "",
                 "benchmark/job/data/akat_movie_id.kdbindex");
    table.Insert("title", Type::Text(), "benchmark/job/data/akat_title.kdb", "",
                 "benchmark/job/data/akat_title.kdbindex");
    table.Insert("imdb_index", Type::Text(),
                 "benchmark/job/data/akat_imdb_index.kdb",
                 "benchmark/job/data/akat_imdb_index_null.kdb",
                 "benchmark/job/data/akat_imdb_index.kdbindex");
    table.Insert("kind_id", Type::Int(), "benchmark/job/data/akat_kind_id.kdb",
                 "", "benchmark/job/data/akat_kind_id.kdbindex");
    table.Insert("production_year", Type::Int(),
                 "benchmark/job/data/akat_production_year.kdb",
                 "benchmark/job/data/akat_production_year_null.kdb",
                 "benchmark/job/data/akat_production_year.kdbindex");
    table.Insert("phonetic_code", Type::Text(),
                 "benchmark/job/data/akat_phonetic_code.kdb",
                 "benchmark/job/data/akat_phonetic_code_null.kdb",
                 "benchmark/job/data/akat_phonetic_code.kdbindex");
    table.Insert("episode_of_id", Type::Int(),
                 "benchmark/job/data/akat_episode_of_id.kdb",
                 "benchmark/job/data/akat_episode_of_id_null.kdb",
                 "benchmark/job/data/akat_episode_of_id.kdbindex");
    table.Insert("season_nr", Type::Int(),
                 "benchmark/job/data/akat_season_nr.kdb",
                 "benchmark/job/data/akat_season_nr_null.kdb",
                 "benchmark/job/data/akat_season_nr.kdbindex");
    table.Insert("episode_nr", Type::Int(),
                 "benchmark/job/data/akat_episode_nr.kdb",
                 "benchmark/job/data/akat_episode_nr_null.kdb",
                 "benchmark/job/data/akat_episode_nr.kdbindex");
    table.Insert("note", Type::Text(), "benchmark/job/data/akat_note.kdb",
                 "benchmark/job/data/akat_note_null.kdb",
                 "benchmark/job/data/akat_note.kdbindex");
    table.Insert("md5sum", Type::Text(), "benchmark/job/data/akat_md5sum.kdb",
                 "benchmark/job/data/akat_md5sum_null.kdb",
                 "benchmark/job/data/akat_md5sum.kdbindex");
  }

  {
    auto& table = db.Insert("cast_info");
    table.Insert("id", Type::Int(), "benchmark/job/data/ci_id.kdb", "",
                 "benchmark/job/data/ci_id.kdbindex");
    table.Insert("person_id", Type::Int(),
                 "benchmark/job/data/ci_person_id.kdb", "",
                 "benchmark/job/data/ci_person_id.kdbindex");
    table.Insert("movie_id", Type::Int(), "benchmark/job/data/ci_movie_id.kdb",
                 "", "benchmark/job/data/ci_movie_id.kdbindex");
    table.Insert("person_role_id", Type::Int(),
                 "benchmark/job/data/ci_person_role_id.kdb",
                 "benchmark/job/data/ci_person_role_id_null.kdb",
                 "benchmark/job/data/ci_person_role_id.kdbindex");
    table.Insert("note", Type::Text(), "benchmark/job/data/ci_note.kdb",
                 "benchmark/job/data/ci_note_null.kdb",
                 "benchmark/job/data/ci_note.kdbindex");
    table.Insert("nr_order", Type::Int(), "benchmark/job/data/ci_nr_order.kdb",
                 "benchmark/job/data/ci_nr_order_null.kdb",
                 "benchmark/job/data/ci_nr_order.kdbindex");
    table.Insert("role_id", Type::Int(), "benchmark/job/data/ci_role_id.kdb",
                 "", "benchmark/job/data/ci_role_id.kdbindex");
  }

  {
    auto& table = db.Insert("char_name");
    table.Insert("id", Type::Int(), "benchmark/job/data/cn_id.kdb", "",
                 "benchmark/job/data/cn_id.kdbindex");
    table.Insert("name", Type::Text(), "benchmark/job/data/cn_name.kdb", "",
                 "benchmark/job/data/cn_name.kdbindex");
    table.Insert("imdb_index", Type::Text(),
                 "benchmark/job/data/cn_imdb_index.kdb",
                 "benchmark/job/data/cn_imdb_index_null.kdb",
                 "benchmark/job/data/cn_imdb_index.kdbindex");
    table.Insert("imdb_id", Type::Int(), "benchmark/job/data/cn_imdb_id.kdb",
                 "benchmark/job/data/cn_imdb_id_null.kdb",
                 "benchmark/job/data/cn_imdb_id.kdbindex");
    table.Insert("name_pcode_nf", Type::Text(),
                 "benchmark/job/data/cn_name_pcode_nf.kdb",
                 "benchmark/job/data/cn_name_pcode_nf_null.kdb",
                 "benchmark/job/data/cn_name_pcode_nf.kdbindex");
    table.Insert("surname_pcode", Type::Text(),
                 "benchmark/job/data/cn_surname_pcode.kdb",
                 "benchmark/job/data/cn_surname_pcode_null.kdb",
                 "benchmark/job/data/cn_surname_pcode.kdbindex");
    table.Insert("md5sum", Type::Text(), "benchmark/job/data/cn_md5sum.kdb",
                 "benchmark/job/data/cn_md5sum_null.kdb",
                 "benchmark/job/data/cn_md5sum.kdbindex");
  }

  {
    auto& table = db.Insert("comp_cast_type");
    table.Insert("id", Type::Int(), "benchmark/job/data/cct_id.kdb", "",
                 "benchmark/job/data/cct_id.kdbindex");
    table.Insert("kind", Type::Text(), "benchmark/job/data/cct_kind.kdb",
                 "benchmark/job/data/cct_kind_null.kdb",
                 "benchmark/job/data/cct_kind.kdbindex");
  }

  {
    auto& table = db.Insert("company_name");
    table.Insert("id", Type::Int(), "benchmark/job/data/cmpn_id.kdb", "",
                 "benchmark/job/data/cmpn_id.kdbindex");
    table.Insert("name", Type::Text(), "benchmark/job/data/cmpn_name.kdb", "",
                 "benchmark/job/data/cmpn_name.kdbindex");
    table.Insert("country_code", Type::Text(),
                 "benchmark/job/data/cmpn_country_code.kdb",
                 "benchmark/job/data/cmpn_country_code_null.kdb",
                 "benchmark/job/data/cmpn_country_code.kdbindex");
    table.Insert("imdb_id", Type::Int(), "benchmark/job/data/cmpn_imdb_id.kdb",
                 "benchmark/job/data/cmpn_imdb_id_null.kdb",
                 "benchmark/job/data/cmpn_imdb_id.kdbindex");
    table.Insert("name_pcode_nf", Type::Text(),
                 "benchmark/job/data/cmpn_name_pcode_nf.kdb",
                 "benchmark/job/data/cmpn_name_pcode_nf_null.kdb",
                 "benchmark/job/data/cmpn_name_pcode_nf.kdbindex");
    table.Insert("surname_pcode", Type::Text(),
                 "benchmark/job/data/cmpn_surname_pcode.kdb",
                 "benchmark/job/data/cmpn_surname_pcode_null.kdb",
                 "benchmark/job/data/cmpn_surname_pcode.kdbindex");
    table.Insert("md5sum", Type::Text(), "benchmark/job/data/cmpn_md5sum.kdb",
                 "benchmark/job/data/cmpn_md5sum_null.kdb",
                 "benchmark/job/data/cmpn_md5sum.kdbindex");
  }

  {
    auto& table = db.Insert("company_type");
    table.Insert("id", Type::Int(), "benchmark/job/data/ct_id.kdb", "",
                 "benchmark/job/data/ct_id.kdbindex");
    table.Insert("kind", Type::Text(), "benchmark/job/data/ct_kind.kdb",
                 "benchmark/job/data/ct_kind_null.kdb",
                 "benchmark/job/data/ct_kind.kdbindex");
  }

  {
    auto& table = db.Insert("complete_cast");
    table.Insert("id", Type::Int(), "benchmark/job/data/cc_id.kdb", "",
                 "benchmark/job/data/cc_id.kdbindex");
    table.Insert("movie_id", Type::Int(), "benchmark/job/data/cc_movie_id.kdb",
                 "benchmark/job/data/cc_movie_id_null.kdb",
                 "benchmark/job/data/cc_movie_id.kdbindex");
    table.Insert("subject_id", Type::Int(),
                 "benchmark/job/data/cc_subject_id.kdb", "",
                 "benchmark/job/data/cc_subject_id.kdbindex");
    table.Insert("status_id", Type::Int(),
                 "benchmark/job/data/cc_status_id.kdb", "",
                 "benchmark/job/data/cc_status_id.kdbindex");
  }

  {
    auto& table = db.Insert("info_type");
    table.Insert("id", Type::Int(), "benchmark/job/data/it_id.kdb", "",
                 "benchmark/job/data/it_id.kdbindex");
    table.Insert("info", Type::Text(), "benchmark/job/data/it_info.kdb",
                 "benchmark/job/data/it_info_null.kdb",
                 "benchmark/job/data/it_info.kdbindex");
  }

  {
    auto& table = db.Insert("keyword");
    table.Insert("id", Type::Int(), "benchmark/job/data/k_id.kdb", "",
                 "benchmark/job/data/k_id.kdbindex");
    table.Insert("keyword", Type::Text(), "benchmark/job/data/k_keyword.kdb",
                 "", "benchmark/job/data/k_keyword.kdbindex");
    table.Insert("phonetic_code", Type::Text(),
                 "benchmark/job/data/k_phonetic_code.kdb",
                 "benchmark/job/data/k_phonetic_code_null.kdb",
                 "benchmark/job/data/k_phonetic_code.kdbindex");
  }

  {
    auto& table = db.Insert("kind_type");
    table.Insert("id", Type::Int(), "benchmark/job/data/kt_id.kdb", "",
                 "benchmark/job/data/kt_id.kdbindex");
    table.Insert("kind", Type::Text(), "benchmark/job/data/kt_kind.kdb",
                 "benchmark/job/data/kt_kind_null.kdb",
                 "benchmark/job/data/kt_kind.kdbindex");
  }

  {
    auto& table = db.Insert("link_type");
    table.Insert("id", Type::Int(), "benchmark/job/data/lt_id.kdb", "",
                 "benchmark/job/data/lt_id.kdbindex");
    table.Insert("link", Type::Text(), "benchmark/job/data/lt_link.kdb",
                 "benchmark/job/data/lt_link_null.kdb",
                 "benchmark/job/data/lt_link.kdbindex");
  }

  {
    auto& table = db.Insert("movie_companies");
    table.Insert("id", Type::Int(), "benchmark/job/data/mc_id.kdb", "",
                 "benchmark/job/data/mc_id.kdbindex");
    table.Insert("movie_id", Type::Int(), "benchmark/job/data/mc_movie_id.kdb",
                 "", "benchmark/job/data/mc_movie_id.kdbindex");
    table.Insert("company_id", Type::Int(),
                 "benchmark/job/data/mc_company_id.kdb", "",
                 "benchmark/job/data/mc_company_id.kdbindex");
    table.Insert("company_type_id", Type::Int(),
                 "benchmark/job/data/mc_company_type_id.kdb", "",
                 "benchmark/job/data/mc_company_type_id.kdbindex");
    table.Insert("note", Type::Text(), "benchmark/job/data/mc_note.kdb",
                 "benchmark/job/data/mc_note_null.kdb",
                 "benchmark/job/data/mc_note.kdbindex");
  }

  {
    auto& table = db.Insert("movie_info");
    table.Insert("id", Type::Int(), "benchmark/job/data/mi_id.kdb", "",
                 "benchmark/job/data/mi_id.kdbindex");
    table.Insert("movie_id", Type::Int(), "benchmark/job/data/mi_movie_id.kdb",
                 "", "benchmark/job/data/mi_movie_id.kdbindex");
    table.Insert("info_type_id", Type::Int(),
                 "benchmark/job/data/mi_info_type_id.kdb", "",
                 "benchmark/job/data/mi_info_type_id.kdbindex");
    table.Insert("info", Type::Text(), "benchmark/job/data/mi_info.kdb", "",
                 "benchmark/job/data/mi_info.kdbindex");
    table.Insert("note", Type::Text(), "benchmark/job/data/mi_note.kdb",
                 "benchmark/job/data/mi_note_null.kdb",
                 "benchmark/job/data/mi_note.kdbindex");
  }

  {
    auto& table = db.Insert("movie_info_idx");
    table.Insert("id", Type::Int(), "benchmark/job/data/mii_id.kdb", "",
                 "benchmark/job/data/mii_id.kdbindex");
    table.Insert("movie_id", Type::Int(), "benchmark/job/data/mii_movie_id.kdb",
                 "", "benchmark/job/data/mii_movie_id.kdbindex");
    table.Insert("info_type_id", Type::Int(),
                 "benchmark/job/data/mii_info_type_id.kdb", "",
                 "benchmark/job/data/mii_info_type_id.kdbindex");
    table.Insert("info", Type::Text(), "benchmark/job/data/mii_info.kdb", "",
                 "benchmark/job/data/mii_info.kdbindex");
    table.Insert("note", Type::Text(), "benchmark/job/data/mii_note.kdb",
                 "benchmark/job/data/mii_note_null.kdb",
                 "benchmark/job/data/mii_note.kdbindex");
  }

  {
    auto& table = db.Insert("movie_keyword");
    table.Insert("id", Type::Int(), "benchmark/job/data/mk_id.kdb", "",
                 "benchmark/job/data/mk_id.kdbindex");
    table.Insert("movie_id", Type::Int(), "benchmark/job/data/mk_movie_id.kdb",
                 "", "benchmark/job/data/mk_movie_id.kdbindex");
    table.Insert("keyword_id", Type::Int(),
                 "benchmark/job/data/mk_keyword_id.kdb", "",
                 "benchmark/job/data/mk_keyword_id.kdbindex");
  }

  {
    auto& table = db.Insert("movie_link");
    table.Insert("id", Type::Int(), "benchmark/job/data/ml_id.kdb", "",
                 "benchmark/job/data/ml_id.kdbindex");
    table.Insert("movie_id", Type::Int(), "benchmark/job/data/ml_movie_id.kdb",
                 "", "benchmark/job/data/ml_movie_id.kdbindex");
    table.Insert("linked_movie_id", Type::Int(),
                 "benchmark/job/data/ml_linked_movie_id.kdb", "",
                 "benchmark/job/data/ml_linked_movie_id.kdbindex");
    table.Insert("link_type_id", Type::Int(),
                 "benchmark/job/data/ml_link_type_id.kdb", "",
                 "benchmark/job/data/ml_link_type_id.kdbindex");
  }

  {
    auto& table = db.Insert("name");
    table.Insert("id", Type::Int(), "benchmark/job/data/n_id.kdb", "",
                 "benchmark/job/data/n_id.kdbindex");
    table.Insert("name", Type::Text(), "benchmark/job/data/n_name.kdb", "",
                 "benchmark/job/data/n_name.kdbindex");
    table.Insert("imdb_index", Type::Text(),
                 "benchmark/job/data/n_imdb_index.kdb",
                 "benchmark/job/data/n_imdb_index_null.kdb",
                 "benchmark/job/data/n_imdb_index.kdbindex");
    table.Insert("imdb_id", Type::Int(), "benchmark/job/data/n_imdb_id.kdb",
                 "benchmark/job/data/n_imdb_id_null.kdb",
                 "benchmark/job/data/n_imdb_id.kdbindex");
    table.Insert("gender", Type::Text(), "benchmark/job/data/n_gender.kdb",
                 "benchmark/job/data/n_gender_null.kdb",
                 "benchmark/job/data/n_gender.kdbindex");
    table.Insert("name_pcode_cf", Type::Text(),
                 "benchmark/job/data/n_name_pcode_cf.kdb",
                 "benchmark/job/data/n_name_pcode_cf_null.kdb",
                 "benchmark/job/data/n_name_pcode_cf.kdbindex");
    table.Insert("name_pcode_nf", Type::Text(),
                 "benchmark/job/data/n_name_pcode_nf.kdb",
                 "benchmark/job/data/n_name_pcode_nf_null.kdb",
                 "benchmark/job/data/n_name_pcode_nf.kdbindex");
    table.Insert("surname_pcode", Type::Text(),
                 "benchmark/job/data/n_surname_pcode.kdb",
                 "benchmark/job/data/n_surname_pcode_null.kdb",
                 "benchmark/job/data/n_surname_pcode.kdbindex");
    table.Insert("md5sum", Type::Text(), "benchmark/job/data/n_md5sum.kdb",
                 "benchmark/job/data/n_md5sum_null.kdb",
                 "benchmark/job/data/n_md5sum.kdbindex");
  }

  {
    auto& table = db.Insert("person_info");
    table.Insert("id", Type::Int(), "benchmark/job/data/pi_id.kdb", "",
                 "benchmark/job/data/pi_id.kdbindex");
    table.Insert("person_id", Type::Int(),
                 "benchmark/job/data/pi_person_id.kdb", "",
                 "benchmark/job/data/pi_person_id.kdbindex");
    table.Insert("info_type_id", Type::Int(),
                 "benchmark/job/data/pi_info_type_id.kdb", "",
                 "benchmark/job/data/pi_info_type_id.kdbindex");
    table.Insert("info", Type::Text(), "benchmark/job/data/pi_info.kdb", "",
                 "benchmark/job/data/pi_info.kdbindex");
    table.Insert("note", Type::Text(), "benchmark/job/data/pi_note.kdb",
                 "benchmark/job/data/pi_note_null.kdb",
                 "benchmark/job/data/pi_note.kdbindex");
  }

  {
    auto& table = db.Insert("role_type");
    table.Insert("id", Type::Int(), "benchmark/job/data/r_id.kdb", "",
                 "benchmark/job/data/r_id.kdbindex");
    table.Insert("role", Type::Text(), "benchmark/job/data/r_role.kdb",
                 "benchmark/job/data/r_role_null.kdb",
                 "benchmark/job/data/r_role.kdbindex");
  }

  {
    auto& table = db.Insert("title");
    table.Insert("id", Type::Int(), "benchmark/job/data/t_id.kdb", "",
                 "benchmark/job/data/t_id.kdbindex");
    table.Insert("title", Type::Text(), "benchmark/job/data/t_title.kdb", "",
                 "benchmark/job/data/t_title.kdbindex");
    table.Insert("imdb_index", Type::Text(),
                 "benchmark/job/data/t_imdb_index.kdb",
                 "benchmark/job/data/t_imdb_index_null.kdb",
                 "benchmark/job/data/t_imdb_index.kdbindex");
    table.Insert("kind_id", Type::Int(), "benchmark/job/data/t_kind_id.kdb", "",
                 "benchmark/job/data/t_kind_id.kdbindex");
    table.Insert("production_year", Type::Int(),
                 "benchmark/job/data/t_production_year.kdb",
                 "benchmark/job/data/t_production_year_null.kdb",
                 "benchmark/job/data/t_production_year.kdbindex");
    table.Insert("imdb_id", Type::Int(), "benchmark/job/data/t_imdb_id.kdb",
                 "benchmark/job/data/t_imdb_id_null.kdb",
                 "benchmark/job/data/t_imdb_id.kdbindex");
    table.Insert("phonetic_code", Type::Text(),
                 "benchmark/job/data/t_phonetic_code.kdb",
                 "benchmark/job/data/t_phonetic_code_null.kdb",
                 "benchmark/job/data/t_phonetic_code.kdbindex");
    table.Insert("episode_of_id", Type::Int(),
                 "benchmark/job/data/t_episode_of_id.kdb",
                 "benchmark/job/data/t_episode_of_id_null.kdb",
                 "benchmark/job/data/t_episode_of_id.kdbindex");
    table.Insert("season_nr", Type::Int(), "benchmark/job/data/t_season_nr.kdb",
                 "benchmark/job/data/t_season_nr_null.kdb",
                 "benchmark/job/data/t_season_nr.kdbindex");
    table.Insert("episode_nr", Type::Int(),
                 "benchmark/job/data/t_episode_nr.kdb",
                 "benchmark/job/data/t_episode_nr_null.kdb",
                 "benchmark/job/data/t_episode_nr.kdbindex");
    table.Insert("series_years", Type::Text(),
                 "benchmark/job/data/t_series_years.kdb",
                 "benchmark/job/data/t_series_years_null.kdb",
                 "benchmark/job/data/t_series_years.kdbindex");
    table.Insert("md5sum", Type::Text(), "benchmark/job/data/t_md5sum.kdb",
                 "benchmark/job/data/t_md5sum_null.kdb",
                 "benchmark/job/data/t_md5sum.kdbindex");
  }

  return db;
}

kush::catalog::Database EnumSchema() {
  using namespace kush::catalog;
  Database db;

  {
    auto& table = db.Insert("aka_name");
    table.Insert("id", Type::Int(), "benchmark/job/data/akan_id.kdb", "",
                 "benchmark/job/data/akan_id.kdbindex");
    table.Insert("person_id", Type::Int(),
                 "benchmark/job/data/akan_person_id.kdb", "",
                 "benchmark/job/data/akan_person_id.kdbindex");
    auto akan_name_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/akan_name_enum.kdbenum");
    table.Insert("name", Type::Enum(akan_name_enum),
                 "benchmark/job/data/akan_name_enum.kdb", "",
                 "benchmark/job/data/akan_name_enum.kdbindex");
    auto akan_imdb_index_enum =
        kush::runtime::Enum::EnumManager::Get().Register(
            "benchmark/job/data/akan_imdb_index_enum.kdbenum");
    table.Insert("imdb_index", Type::Enum(akan_imdb_index_enum),
                 "benchmark/job/data/akan_imdb_index_enum.kdb",
                 "benchmark/job/data/akan_imdb_index_enum_null.kdb",
                 "benchmark/job/data/akan_imdb_index_enum.kdbindex");
    auto akan_name_pcode_cf_enum =
        kush::runtime::Enum::EnumManager::Get().Register(
            "benchmark/job/data/akan_name_pcode_cf_enum.kdbenum");
    table.Insert("name_pcode_cf", Type::Enum(akan_name_pcode_cf_enum),
                 "benchmark/job/data/akan_name_pcode_cf_enum.kdb",
                 "benchmark/job/data/akan_name_pcode_cf_enum_null.kdb",
                 "benchmark/job/data/akan_name_pcode_cf_enum.kdbindex");
    auto akan_name_pcode_nf_enum =
        kush::runtime::Enum::EnumManager::Get().Register(
            "benchmark/job/data/akan_name_pcode_nf_enum.kdbenum");
    table.Insert("name_pcode_nf", Type::Enum(akan_name_pcode_nf_enum),
                 "benchmark/job/data/akan_name_pcode_nf_enum.kdb",
                 "benchmark/job/data/akan_name_pcode_nf_enum_null.kdb",
                 "benchmark/job/data/akan_name_pcode_nf_enum.kdbindex");
    auto akan_surname_pcode_enum =
        kush::runtime::Enum::EnumManager::Get().Register(
            "benchmark/job/data/akan_surname_pcode_enum.kdbenum");
    table.Insert("surname_pcode", Type::Enum(akan_surname_pcode_enum),
                 "benchmark/job/data/akan_surname_pcode_enum.kdb",
                 "benchmark/job/data/akan_surname_pcode_enum_null.kdb",
                 "benchmark/job/data/akan_surname_pcode_enum.kdbindex");
    auto akan_md5sum_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/akan_md5sum_enum.kdbenum");
    table.Insert("md5sum", Type::Enum(akan_md5sum_enum),
                 "benchmark/job/data/akan_md5sum_enum.kdb",
                 "benchmark/job/data/akan_md5sum_enum_null.kdb",
                 "benchmark/job/data/akan_md5sum_enum.kdbindex");
  }

  {
    auto& table = db.Insert("aka_title");
    table.Insert("id", Type::Int(), "benchmark/job/data/akat_id.kdb", "",
                 "benchmark/job/data/akat_id.kdbindex");
    table.Insert("movie_id", Type::Int(),
                 "benchmark/job/data/akat_movie_id.kdb", "",
                 "benchmark/job/data/akat_movie_id.kdbindex");
    auto akat_title_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/akat_title_enum.kdbenum");
    table.Insert("title", Type::Enum(akat_title_enum),
                 "benchmark/job/data/akat_title_enum.kdb", "",
                 "benchmark/job/data/akat_title_enum.kdbindex");
    auto akat_imdb_index_enum =
        kush::runtime::Enum::EnumManager::Get().Register(
            "benchmark/job/data/akat_imdb_index_enum.kdbenum");
    table.Insert("imdb_index", Type::Enum(akat_imdb_index_enum),
                 "benchmark/job/data/akat_imdb_index_enum.kdb",
                 "benchmark/job/data/akat_imdb_index_enum_null.kdb",
                 "benchmark/job/data/akat_imdb_index_enum.kdbindex");
    table.Insert("kind_id", Type::Int(), "benchmark/job/data/akat_kind_id.kdb",
                 "", "benchmark/job/data/akat_kind_id.kdbindex");
    table.Insert("production_year", Type::Int(),
                 "benchmark/job/data/akat_production_year.kdb",
                 "benchmark/job/data/akat_production_year_null.kdb",
                 "benchmark/job/data/akat_production_year.kdbindex");
    auto akat_phonetic_code_enum =
        kush::runtime::Enum::EnumManager::Get().Register(
            "benchmark/job/data/akat_phonetic_code_enum.kdbenum");
    table.Insert("phonetic_code", Type::Enum(akat_phonetic_code_enum),
                 "benchmark/job/data/akat_phonetic_code_enum.kdb",
                 "benchmark/job/data/akat_phonetic_code_enum_null.kdb",
                 "benchmark/job/data/akat_phonetic_code_enum.kdbindex");
    table.Insert("episode_of_id", Type::Int(),
                 "benchmark/job/data/akat_episode_of_id.kdb",
                 "benchmark/job/data/akat_episode_of_id_null.kdb",
                 "benchmark/job/data/akat_episode_of_id.kdbindex");
    table.Insert("season_nr", Type::Int(),
                 "benchmark/job/data/akat_season_nr.kdb",
                 "benchmark/job/data/akat_season_nr_null.kdb",
                 "benchmark/job/data/akat_season_nr.kdbindex");
    table.Insert("episode_nr", Type::Int(),
                 "benchmark/job/data/akat_episode_nr.kdb",
                 "benchmark/job/data/akat_episode_nr_null.kdb",
                 "benchmark/job/data/akat_episode_nr.kdbindex");
    auto akat_note_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/akat_note_enum.kdbenum");
    table.Insert("note", Type::Enum(akat_note_enum),
                 "benchmark/job/data/akat_note_enum.kdb",
                 "benchmark/job/data/akat_note_enum_null.kdb",
                 "benchmark/job/data/akat_note_enum.kdbindex");
    auto akat_md5sum_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/akat_md5sum_enum.kdbenum");
    table.Insert("md5sum", Type::Enum(akat_md5sum_enum),
                 "benchmark/job/data/akat_md5sum_enum.kdb",
                 "benchmark/job/data/akat_md5sum_enum_null.kdb",
                 "benchmark/job/data/akat_md5sum_enum.kdbindex");
  }

  {
    auto& table = db.Insert("cast_info");
    table.Insert("id", Type::Int(), "benchmark/job/data/ci_id.kdb", "",
                 "benchmark/job/data/ci_id.kdbindex");
    table.Insert("person_id", Type::Int(),
                 "benchmark/job/data/ci_person_id.kdb", "",
                 "benchmark/job/data/ci_person_id.kdbindex");
    table.Insert("movie_id", Type::Int(), "benchmark/job/data/ci_movie_id.kdb",
                 "", "benchmark/job/data/ci_movie_id.kdbindex");
    table.Insert("person_role_id", Type::Int(),
                 "benchmark/job/data/ci_person_role_id.kdb",
                 "benchmark/job/data/ci_person_role_id_null.kdb",
                 "benchmark/job/data/ci_person_role_id.kdbindex");
    auto ci_note_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/ci_note_enum.kdbenum");
    table.Insert("note", Type::Enum(ci_note_enum),
                 "benchmark/job/data/ci_note_enum.kdb",
                 "benchmark/job/data/ci_note_enum_null.kdb",
                 "benchmark/job/data/ci_note_enum.kdbindex");
    table.Insert("nr_order", Type::Int(), "benchmark/job/data/ci_nr_order.kdb",
                 "benchmark/job/data/ci_nr_order_null.kdb",
                 "benchmark/job/data/ci_nr_order.kdbindex");
    table.Insert("role_id", Type::Int(), "benchmark/job/data/ci_role_id.kdb",
                 "", "benchmark/job/data/ci_role_id.kdbindex");
  }

  {
    auto& table = db.Insert("char_name");
    table.Insert("id", Type::Int(), "benchmark/job/data/cn_id.kdb", "",
                 "benchmark/job/data/cn_id.kdbindex");
    auto cn_name_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/cn_name_enum.kdbenum");
    table.Insert("name", Type::Enum(cn_name_enum),
                 "benchmark/job/data/cn_name_enum.kdb", "",
                 "benchmark/job/data/cn_name_enum.kdbindex");
    auto cn_imdb_index_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/cn_imdb_index_enum.kdbenum");
    table.Insert("imdb_index", Type::Enum(cn_imdb_index_enum),
                 "benchmark/job/data/cn_imdb_index_enum.kdb",
                 "benchmark/job/data/cn_imdb_index_enum_null.kdb",
                 "benchmark/job/data/cn_imdb_index_enum.kdbindex");
    table.Insert("imdb_id", Type::Int(), "benchmark/job/data/cn_imdb_id.kdb",
                 "benchmark/job/data/cn_imdb_id_null.kdb",
                 "benchmark/job/data/cn_imdb_id.kdbindex");
    auto cn_name_pcode_nf_enum =
        kush::runtime::Enum::EnumManager::Get().Register(
            "benchmark/job/data/cn_name_pcode_nf_enum.kdbenum");
    table.Insert("name_pcode_nf", Type::Enum(cn_name_pcode_nf_enum),
                 "benchmark/job/data/cn_name_pcode_nf_enum.kdb",
                 "benchmark/job/data/cn_name_pcode_nf_enum_null.kdb",
                 "benchmark/job/data/cn_name_pcode_nf_enum.kdbindex");
    auto cn_surname_pcode_enum =
        kush::runtime::Enum::EnumManager::Get().Register(
            "benchmark/job/data/cn_surname_pcode_enum.kdbenum");
    table.Insert("surname_pcode", Type::Enum(cn_surname_pcode_enum),
                 "benchmark/job/data/cn_surname_pcode_enum.kdb",
                 "benchmark/job/data/cn_surname_pcode_enum_null.kdb",
                 "benchmark/job/data/cn_surname_pcode_enum.kdbindex");
    auto cn_md5sum_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/cn_md5sum_enum.kdbenum");
    table.Insert("md5sum", Type::Enum(cn_md5sum_enum),
                 "benchmark/job/data/cn_md5sum_enum.kdb",
                 "benchmark/job/data/cn_md5sum_enum_null.kdb",
                 "benchmark/job/data/cn_md5sum_enum.kdbindex");
  }

  {
    auto& table = db.Insert("comp_cast_type");
    table.Insert("id", Type::Int(), "benchmark/job/data/cct_id.kdb", "",
                 "benchmark/job/data/cct_id.kdbindex");
    auto cct_kind_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/cct_kind_enum.kdbenum");
    table.Insert("kind", Type::Enum(cct_kind_enum),
                 "benchmark/job/data/cct_kind_enum.kdb",
                 "benchmark/job/data/cct_kind_enum_null.kdb",
                 "benchmark/job/data/cct_kind_enum.kdbindex");
  }

  {
    auto& table = db.Insert("company_name");
    table.Insert("id", Type::Int(), "benchmark/job/data/cmpn_id.kdb", "",
                 "benchmark/job/data/cmpn_id.kdbindex");
    auto cmpn_name_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/cmpn_name_enum.kdbenum");
    table.Insert("name", Type::Enum(cmpn_name_enum),
                 "benchmark/job/data/cmpn_name_enum.kdb", "",
                 "benchmark/job/data/cmpn_name_enum.kdbindex");
    auto cmpn_country_code_enum =
        kush::runtime::Enum::EnumManager::Get().Register(
            "benchmark/job/data/cmpn_country_code_enum.kdbenum");
    table.Insert("country_code", Type::Enum(cmpn_country_code_enum),
                 "benchmark/job/data/cmpn_country_code_enum.kdb",
                 "benchmark/job/data/cmpn_country_code_enum_null.kdb",
                 "benchmark/job/data/cmpn_country_code_enum.kdbindex");
    table.Insert("imdb_id", Type::Int(), "benchmark/job/data/cmpn_imdb_id.kdb",
                 "benchmark/job/data/cmpn_imdb_id_null.kdb",
                 "benchmark/job/data/cmpn_imdb_id.kdbindex");
    auto cmpn_name_pcode_nf_enum =
        kush::runtime::Enum::EnumManager::Get().Register(
            "benchmark/job/data/cmpn_name_pcode_nf_enum.kdbenum");
    table.Insert("name_pcode_nf", Type::Enum(cmpn_name_pcode_nf_enum),
                 "benchmark/job/data/cmpn_name_pcode_nf_enum.kdb",
                 "benchmark/job/data/cmpn_name_pcode_nf_enum_null.kdb",
                 "benchmark/job/data/cmpn_name_pcode_nf_enum.kdbindex");
    auto cmpn_surname_pcode_enum =
        kush::runtime::Enum::EnumManager::Get().Register(
            "benchmark/job/data/cmpn_surname_pcode_enum.kdbenum");
    table.Insert("surname_pcode", Type::Enum(cmpn_surname_pcode_enum),
                 "benchmark/job/data/cmpn_surname_pcode_enum.kdb",
                 "benchmark/job/data/cmpn_surname_pcode_enum_null.kdb",
                 "benchmark/job/data/cmpn_surname_pcode_enum.kdbindex");
    auto cmpn_md5sum_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/cmpn_md5sum_enum.kdbenum");
    table.Insert("md5sum", Type::Enum(cmpn_md5sum_enum),
                 "benchmark/job/data/cmpn_md5sum_enum.kdb",
                 "benchmark/job/data/cmpn_md5sum_enum_null.kdb",
                 "benchmark/job/data/cmpn_md5sum_enum.kdbindex");
  }

  {
    auto& table = db.Insert("company_type");
    table.Insert("id", Type::Int(), "benchmark/job/data/ct_id.kdb", "",
                 "benchmark/job/data/ct_id.kdbindex");
    auto ct_kind_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/ct_kind_enum.kdbenum");
    table.Insert("kind", Type::Enum(ct_kind_enum),
                 "benchmark/job/data/ct_kind_enum.kdb",
                 "benchmark/job/data/ct_kind_enum_null.kdb",
                 "benchmark/job/data/ct_kind_enum.kdbindex");
  }

  {
    auto& table = db.Insert("complete_cast");
    table.Insert("id", Type::Int(), "benchmark/job/data/cc_id.kdb", "",
                 "benchmark/job/data/cc_id.kdbindex");
    table.Insert("movie_id", Type::Int(), "benchmark/job/data/cc_movie_id.kdb",
                 "benchmark/job/data/cc_movie_id_null.kdb",
                 "benchmark/job/data/cc_movie_id.kdbindex");
    table.Insert("subject_id", Type::Int(),
                 "benchmark/job/data/cc_subject_id.kdb", "",
                 "benchmark/job/data/cc_subject_id.kdbindex");
    table.Insert("status_id", Type::Int(),
                 "benchmark/job/data/cc_status_id.kdb", "",
                 "benchmark/job/data/cc_status_id.kdbindex");
  }

  {
    auto& table = db.Insert("info_type");
    table.Insert("id", Type::Int(), "benchmark/job/data/it_id.kdb", "",
                 "benchmark/job/data/it_id.kdbindex");
    auto it_info_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/it_info_enum.kdbenum");
    table.Insert("info", Type::Enum(it_info_enum),
                 "benchmark/job/data/it_info_enum.kdb",
                 "benchmark/job/data/it_info_enum_null.kdb",
                 "benchmark/job/data/it_info_enum.kdbindex");
  }

  {
    auto& table = db.Insert("keyword");
    table.Insert("id", Type::Int(), "benchmark/job/data/k_id.kdb", "",
                 "benchmark/job/data/k_id.kdbindex");
    auto k_keyword_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/k_keyword_enum.kdbenum");
    table.Insert("keyword", Type::Enum(k_keyword_enum),
                 "benchmark/job/data/k_keyword_enum.kdb", "",
                 "benchmark/job/data/k_keyword_enum.kdbindex");
    auto k_phonetic_code_enum =
        kush::runtime::Enum::EnumManager::Get().Register(
            "benchmark/job/data/k_phonetic_code_enum.kdbenum");
    table.Insert("phonetic_code", Type::Enum(k_phonetic_code_enum),
                 "benchmark/job/data/k_phonetic_code_enum.kdb",
                 "benchmark/job/data/k_phonetic_code_enum_null.kdb",
                 "benchmark/job/data/k_phonetic_code_enum.kdbindex");
  }

  {
    auto& table = db.Insert("kind_type");
    table.Insert("id", Type::Int(), "benchmark/job/data/kt_id.kdb", "",
                 "benchmark/job/data/kt_id.kdbindex");
    auto kt_kind_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/kt_kind_enum.kdbenum");
    table.Insert("kind", Type::Enum(kt_kind_enum),
                 "benchmark/job/data/kt_kind_enum.kdb",
                 "benchmark/job/data/kt_kind_enum_null.kdb",
                 "benchmark/job/data/kt_kind_enum.kdbindex");
  }

  {
    auto& table = db.Insert("link_type");
    table.Insert("id", Type::Int(), "benchmark/job/data/lt_id.kdb", "",
                 "benchmark/job/data/lt_id.kdbindex");
    auto lt_link_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/lt_link_enum.kdbenum");
    table.Insert("link", Type::Enum(lt_link_enum),
                 "benchmark/job/data/lt_link_enum.kdb",
                 "benchmark/job/data/lt_link_enum_null.kdb",
                 "benchmark/job/data/lt_link_enum.kdbindex");
  }

  {
    auto& table = db.Insert("movie_companies");
    table.Insert("id", Type::Int(), "benchmark/job/data/mc_id.kdb", "",
                 "benchmark/job/data/mc_id.kdbindex");
    table.Insert("movie_id", Type::Int(), "benchmark/job/data/mc_movie_id.kdb",
                 "", "benchmark/job/data/mc_movie_id.kdbindex");
    table.Insert("company_id", Type::Int(),
                 "benchmark/job/data/mc_company_id.kdb", "",
                 "benchmark/job/data/mc_company_id.kdbindex");
    table.Insert("company_type_id", Type::Int(),
                 "benchmark/job/data/mc_company_type_id.kdb", "",
                 "benchmark/job/data/mc_company_type_id.kdbindex");
    auto mc_note_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/mc_note_enum.kdbenum");
    table.Insert("note", Type::Enum(mc_note_enum),
                 "benchmark/job/data/mc_note_enum.kdb",
                 "benchmark/job/data/mc_note_enum_null.kdb",
                 "benchmark/job/data/mc_note_enum.kdbindex");
  }

  {
    auto& table = db.Insert("movie_info");
    table.Insert("id", Type::Int(), "benchmark/job/data/mi_id.kdb", "",
                 "benchmark/job/data/mi_id.kdbindex");
    table.Insert("movie_id", Type::Int(), "benchmark/job/data/mi_movie_id.kdb",
                 "", "benchmark/job/data/mi_movie_id.kdbindex");
    table.Insert("info_type_id", Type::Int(),
                 "benchmark/job/data/mi_info_type_id.kdb", "",
                 "benchmark/job/data/mi_info_type_id.kdbindex");
    auto mi_info_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/mi_info_enum.kdbenum");
    table.Insert("info", Type::Enum(mi_info_enum),
                 "benchmark/job/data/mi_info_enum.kdb", "",
                 "benchmark/job/data/mi_info_enum.kdbindex");
    auto mi_note_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/mi_note_enum.kdbenum");
    table.Insert("note", Type::Enum(mi_note_enum),
                 "benchmark/job/data/mi_note_enum.kdb",
                 "benchmark/job/data/mi_note_enum_null.kdb",
                 "benchmark/job/data/mi_note_enum.kdbindex");
  }

  {
    auto& table = db.Insert("movie_info_idx");
    table.Insert("id", Type::Int(), "benchmark/job/data/mii_id.kdb", "",
                 "benchmark/job/data/mii_id.kdbindex");
    table.Insert("movie_id", Type::Int(), "benchmark/job/data/mii_movie_id.kdb",
                 "", "benchmark/job/data/mii_movie_id.kdbindex");
    table.Insert("info_type_id", Type::Int(),
                 "benchmark/job/data/mii_info_type_id.kdb", "",
                 "benchmark/job/data/mii_info_type_id.kdbindex");
    auto mii_info_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/mii_info_enum.kdbenum");
    table.Insert("info", Type::Enum(mii_info_enum),
                 "benchmark/job/data/mii_info_enum.kdb", "",
                 "benchmark/job/data/mii_info_enum.kdbindex");
    auto mii_note_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/mii_note_enum.kdbenum");
    table.Insert("note", Type::Enum(mii_note_enum),
                 "benchmark/job/data/mii_note_enum.kdb",
                 "benchmark/job/data/mii_note_enum_null.kdb",
                 "benchmark/job/data/mii_note_enum.kdbindex");
  }

  {
    auto& table = db.Insert("movie_keyword");
    table.Insert("id", Type::Int(), "benchmark/job/data/mk_id.kdb", "",
                 "benchmark/job/data/mk_id.kdbindex");
    table.Insert("movie_id", Type::Int(), "benchmark/job/data/mk_movie_id.kdb",
                 "", "benchmark/job/data/mk_movie_id.kdbindex");
    table.Insert("keyword_id", Type::Int(),
                 "benchmark/job/data/mk_keyword_id.kdb", "",
                 "benchmark/job/data/mk_keyword_id.kdbindex");
  }

  {
    auto& table = db.Insert("movie_link");
    table.Insert("id", Type::Int(), "benchmark/job/data/ml_id.kdb", "",
                 "benchmark/job/data/ml_id.kdbindex");
    table.Insert("movie_id", Type::Int(), "benchmark/job/data/ml_movie_id.kdb",
                 "", "benchmark/job/data/ml_movie_id.kdbindex");
    table.Insert("linked_movie_id", Type::Int(),
                 "benchmark/job/data/ml_linked_movie_id.kdb", "",
                 "benchmark/job/data/ml_linked_movie_id.kdbindex");
    table.Insert("link_type_id", Type::Int(),
                 "benchmark/job/data/ml_link_type_id.kdb", "",
                 "benchmark/job/data/ml_link_type_id.kdbindex");
  }

  {
    auto& table = db.Insert("name");
    table.Insert("id", Type::Int(), "benchmark/job/data/n_id.kdb", "",
                 "benchmark/job/data/n_id.kdbindex");
    auto n_name_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/n_name_enum.kdbenum");
    table.Insert("name", Type::Enum(n_name_enum),
                 "benchmark/job/data/n_name_enum.kdb", "",
                 "benchmark/job/data/n_name_enum.kdbindex");
    auto n_imdb_index_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/n_imdb_index_enum.kdbenum");
    table.Insert("imdb_index", Type::Enum(n_imdb_index_enum),
                 "benchmark/job/data/n_imdb_index_enum.kdb",
                 "benchmark/job/data/n_imdb_index_enum_null.kdb",
                 "benchmark/job/data/n_imdb_index_enum.kdbindex");
    table.Insert("imdb_id", Type::Int(), "benchmark/job/data/n_imdb_id.kdb",
                 "benchmark/job/data/n_imdb_id_null.kdb",
                 "benchmark/job/data/n_imdb_id.kdbindex");
    auto n_gender_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/n_gender_enum.kdbenum");
    table.Insert("gender", Type::Enum(n_gender_enum),
                 "benchmark/job/data/n_gender_enum.kdb",
                 "benchmark/job/data/n_gender_enum_null.kdb",
                 "benchmark/job/data/n_gender_enum.kdbindex");
    auto n_name_pcode_cf_enum =
        kush::runtime::Enum::EnumManager::Get().Register(
            "benchmark/job/data/n_name_pcode_cf_enum.kdbenum");
    table.Insert("name_pcode_cf", Type::Enum(n_name_pcode_cf_enum),
                 "benchmark/job/data/n_name_pcode_cf_enum.kdb",
                 "benchmark/job/data/n_name_pcode_cf_enum_null.kdb",
                 "benchmark/job/data/n_name_pcode_cf_enum.kdbindex");
    auto n_name_pcode_nf_enum =
        kush::runtime::Enum::EnumManager::Get().Register(
            "benchmark/job/data/n_name_pcode_nf_enum.kdbenum");
    table.Insert("name_pcode_nf", Type::Enum(n_name_pcode_nf_enum),
                 "benchmark/job/data/n_name_pcode_nf_enum.kdb",
                 "benchmark/job/data/n_name_pcode_nf_enum_null.kdb",
                 "benchmark/job/data/n_name_pcode_nf_enum.kdbindex");
    auto n_surname_pcode_enum =
        kush::runtime::Enum::EnumManager::Get().Register(
            "benchmark/job/data/n_surname_pcode_enum.kdbenum");
    table.Insert("surname_pcode", Type::Enum(n_surname_pcode_enum),
                 "benchmark/job/data/n_surname_pcode_enum.kdb",
                 "benchmark/job/data/n_surname_pcode_enum_null.kdb",
                 "benchmark/job/data/n_surname_pcode_enum.kdbindex");
    auto n_md5sum_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/n_md5sum_enum.kdbenum");
    table.Insert("md5sum", Type::Enum(n_md5sum_enum),
                 "benchmark/job/data/n_md5sum_enum.kdb",
                 "benchmark/job/data/n_md5sum_enum_null.kdb",
                 "benchmark/job/data/n_md5sum_enum.kdbindex");
  }

  {
    auto& table = db.Insert("person_info");
    table.Insert("id", Type::Int(), "benchmark/job/data/pi_id.kdb", "",
                 "benchmark/job/data/pi_id.kdbindex");
    table.Insert("person_id", Type::Int(),
                 "benchmark/job/data/pi_person_id.kdb", "",
                 "benchmark/job/data/pi_person_id.kdbindex");
    table.Insert("info_type_id", Type::Int(),
                 "benchmark/job/data/pi_info_type_id.kdb", "",
                 "benchmark/job/data/pi_info_type_id.kdbindex");
    auto pi_info_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/pi_info_enum.kdbenum");
    table.Insert("info", Type::Enum(pi_info_enum),
                 "benchmark/job/data/pi_info_enum.kdb", "",
                 "benchmark/job/data/pi_info_enum.kdbindex");
    auto pi_note_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/pi_note_enum.kdbenum");
    table.Insert("note", Type::Enum(pi_note_enum),
                 "benchmark/job/data/pi_note_enum.kdb",
                 "benchmark/job/data/pi_note_enum_null.kdb",
                 "benchmark/job/data/pi_note_enum.kdbindex");
  }

  {
    auto& table = db.Insert("role_type");
    table.Insert("id", Type::Int(), "benchmark/job/data/r_id.kdb", "",
                 "benchmark/job/data/r_id.kdbindex");
    auto r_role_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/r_role_enum.kdbenum");
    table.Insert("role", Type::Enum(r_role_enum),
                 "benchmark/job/data/r_role_enum.kdb",
                 "benchmark/job/data/r_role_enum_null.kdb",
                 "benchmark/job/data/r_role_enum.kdbindex");
  }

  {
    auto& table = db.Insert("title");
    table.Insert("id", Type::Int(), "benchmark/job/data/t_id.kdb", "",
                 "benchmark/job/data/t_id.kdbindex");
    auto t_title_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/t_title_enum.kdbenum");
    table.Insert("title", Type::Enum(t_title_enum),
                 "benchmark/job/data/t_title_enum.kdb", "",
                 "benchmark/job/data/t_title_enum.kdbindex");
    auto t_imdb_index_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/t_imdb_index_enum.kdbenum");
    table.Insert("imdb_index", Type::Enum(t_imdb_index_enum),
                 "benchmark/job/data/t_imdb_index_enum.kdb",
                 "benchmark/job/data/t_imdb_index_enum_null.kdb",
                 "benchmark/job/data/t_imdb_index_enum.kdbindex");
    table.Insert("kind_id", Type::Int(), "benchmark/job/data/t_kind_id.kdb", "",
                 "benchmark/job/data/t_kind_id.kdbindex");
    table.Insert("production_year", Type::Int(),
                 "benchmark/job/data/t_production_year.kdb",
                 "benchmark/job/data/t_production_year_null.kdb",
                 "benchmark/job/data/t_production_year.kdbindex");
    table.Insert("imdb_id", Type::Int(), "benchmark/job/data/t_imdb_id.kdb",
                 "benchmark/job/data/t_imdb_id_null.kdb",
                 "benchmark/job/data/t_imdb_id.kdbindex");
    auto t_phonetic_code_enum =
        kush::runtime::Enum::EnumManager::Get().Register(
            "benchmark/job/data/t_phonetic_code_enum.kdbenum");
    table.Insert("phonetic_code", Type::Enum(t_phonetic_code_enum),
                 "benchmark/job/data/t_phonetic_code_enum.kdb",
                 "benchmark/job/data/t_phonetic_code_enum_null.kdb",
                 "benchmark/job/data/t_phonetic_code_enum.kdbindex");
    table.Insert("episode_of_id", Type::Int(),
                 "benchmark/job/data/t_episode_of_id.kdb",
                 "benchmark/job/data/t_episode_of_id_null.kdb",
                 "benchmark/job/data/t_episode_of_id.kdbindex");
    table.Insert("season_nr", Type::Int(), "benchmark/job/data/t_season_nr.kdb",
                 "benchmark/job/data/t_season_nr_null.kdb",
                 "benchmark/job/data/t_season_nr.kdbindex");
    table.Insert("episode_nr", Type::Int(),
                 "benchmark/job/data/t_episode_nr.kdb",
                 "benchmark/job/data/t_episode_nr_null.kdb",
                 "benchmark/job/data/t_episode_nr.kdbindex");
    auto t_series_years_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/t_series_years_enum.kdbenum");
    table.Insert("series_years", Type::Enum(t_series_years_enum),
                 "benchmark/job/data/t_series_years_enum.kdb",
                 "benchmark/job/data/t_series_years_enum_null.kdb",
                 "benchmark/job/data/t_series_years_enum.kdbindex");
    auto t_md5sum_enum = kush::runtime::Enum::EnumManager::Get().Register(
        "benchmark/job/data/t_md5sum_enum.kdbenum");
    table.Insert("md5sum", Type::Enum(t_md5sum_enum),
                 "benchmark/job/data/t_md5sum_enum.kdb",
                 "benchmark/job/data/t_md5sum_enum_null.kdb",
                 "benchmark/job/data/t_md5sum_enum.kdbindex");
  }

  return db;
}

kush::catalog::Database Schema() {
  if (FLAGS_use_dictionary.Get()) {
    return EnumSchema();
  }
  return RegularSchema();
}