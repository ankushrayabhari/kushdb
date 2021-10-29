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

  return db;
}